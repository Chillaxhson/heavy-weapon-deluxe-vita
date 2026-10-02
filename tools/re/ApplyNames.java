// Renames functions listed in names.txt (same directory as this script) and gives the
// MSVC __ftol helper its real signature (input in ST0, 64-bit result in EDX:EAX) so the
// decompiler keeps the x87 expressions feeding it.
import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.data.*;
import ghidra.program.model.listing.*;
import ghidra.program.model.symbol.SourceType;
import java.io.*;
import java.nio.file.*;

public class ApplyNames extends GhidraScript {
    @Override
    public void run() throws Exception {
        Path names = Paths.get(getSourceFile().getParentFile().getAbsolutePath(), "names.txt");
        for (String line : Files.readAllLines(names)) {
            line = line.trim();
            if (line.isEmpty() || line.startsWith("#")) continue;
            String[] p = line.split("\\s+");
            Address a = toAddr(Long.parseLong(p[0], 16));
            Function f = getFunctionAt(a);
            if (f == null) f = createFunction(a, null);
            if (f == null) { println("no function at " + p[0]); continue; }
            f.setName(p[1], SourceType.USER_DEFINED);
        }

        Function ftol = getFunctionAt(toAddr(0x4fb4c8L));
        ftol.setCustomVariableStorage(true);
        Parameter in = new ParameterImpl("value", DoubleDataType.dataType,
                currentProgram.getRegister("ST0"), currentProgram);
        ftol.replaceParameters(Function.FunctionUpdateType.CUSTOM_STORAGE, true, SourceType.USER_DEFINED, in);
        ReturnParameterImpl ret = new ReturnParameterImpl(LongLongDataType.dataType,
                new VariableStorage(currentProgram, currentProgram.getRegister("EAX"), currentProgram.getRegister("EDX")),
                currentProgram);
        ftol.setReturn(ret.getDataType(), ret.getVariableStorage(), SourceType.USER_DEFINED);

        // _CIpow(x, y) = x^y with x in ST1 and y in ST0, result in ST0.
        Function pow = getFunctionAt(toAddr(0x4fbd80L));
        pow.setName("_CIpow", SourceType.USER_DEFINED);
        pow.setCustomVariableStorage(true);
        Parameter px = new ParameterImpl("x", DoubleDataType.dataType, currentProgram.getRegister("ST1"), currentProgram);
        Parameter py = new ParameterImpl("y", DoubleDataType.dataType, currentProgram.getRegister("ST0"), currentProgram);
        pow.replaceParameters(Function.FunctionUpdateType.CUSTOM_STORAGE, true, SourceType.USER_DEFINED, px, py);
        pow.setReturn(DoubleDataType.dataType,
                new VariableStorage(currentProgram, currentProgram.getRegister("ST0")), SourceType.USER_DEFINED);
        println("applied names, __ftol and _CIpow signatures");
    }
}
