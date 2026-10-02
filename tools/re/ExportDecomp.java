// Ghidra headless post-script: writes the decompiled C of every function to
// <outdir>/decomp.c (sorted by address) and string cross-references to strings.txt.
// Usage: -postScript ExportDecomp.java <outdir>
import ghidra.app.script.GhidraScript;
import ghidra.app.decompiler.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.data.StringDataInstance;
import ghidra.program.model.symbol.Reference;
import java.io.*;

public class ExportDecomp extends GhidraScript {
    @Override
    public void run() throws Exception {
        File outDir = new File(getScriptArgs()[0]);
        outDir.mkdirs();

        DecompInterface ifc = new DecompInterface();
        DecompileOptions opts = new DecompileOptions();
        ifc.setOptions(opts);
        ifc.openProgram(currentProgram);

        try (PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(new File(outDir, "decomp.c"))))) {
            FunctionIterator it = currentProgram.getFunctionManager().getFunctions(true);
            int n = 0;
            while (it.hasNext() && !monitor.isCancelled()) {
                Function f = it.next();
                DecompileResults res = ifc.decompileFunction(f, 120, monitor);
                out.println("// ==== " + f.getName() + " @ " + f.getEntryPoint() + " ====");
                if (res != null && res.decompileCompleted()) {
                    out.println(res.getDecompiledFunction().getC());
                } else {
                    out.println("// decompile failed: " + (res == null ? "null" : res.getErrorMessage()));
                }
                if (++n % 500 == 0) println("decompiled " + n);
            }
            println("decompiled total " + n);
        }

        try (PrintWriter out = new PrintWriter(new BufferedWriter(new FileWriter(new File(outDir, "strings.txt"))))) {
            DataIterator di = currentProgram.getListing().getDefinedData(true);
            while (di.hasNext()) {
                Data d = di.next();
                if (!d.hasStringValue()) continue;
                StringBuilder refs = new StringBuilder();
                for (Reference r : getReferencesTo(d.getAddress())) {
                    Function f = getFunctionContaining(r.getFromAddress());
                    refs.append(' ').append(f != null ? f.getName() : r.getFromAddress().toString());
                }
                String v = StringDataInstance.getStringDataInstance(d).getStringValue();
                if (v == null) continue;
                out.println(d.getAddress() + "\t" + v.replace("\n", "\\n") + "\t<-" + refs);
            }
        }
    }
}
