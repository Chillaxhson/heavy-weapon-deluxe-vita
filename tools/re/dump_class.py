#!/usr/bin/env python3
"""Dumps a class's constructor and every function in its vtable(s) into one file.
Usage: dump_class.py NAME CTOR_ADDR [CTOR_ADDR...]  -> re/ghidra-out/classes/NAME.c
The vtable is the PTR_FUN_xxxxxxxx assigned to *this in the constructor."""
import os, re, struct, sys

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
OUT = os.path.join(ROOT, 're', 'ghidra-out')
data = open(os.path.join(ROOT, 're', 'inner_game.exe'), 'rb').read()

def func_src(addr):
    p = os.path.join(OUT, 'funcs', '%08x.c' % addr)
    return open(p).read() if os.path.exists(p) else None

MAX_SLOTS = int(os.environ.get('SLOTS', '9'))   # craft vtables have 9 slots

def vtable_entries(va):
    out = []
    off = va - 0x400000
    while len(out) < MAX_SLOTS:
        v = struct.unpack('<I', data[off:off + 4])[0]
        if not (0x401000 <= v < 0x527000):
            break
        out.append(v)
        off += 4
    return out

name, ctors = sys.argv[1], [int(a, 16) for a in sys.argv[2:]]
os.makedirs(os.path.join(OUT, 'classes'), exist_ok=True)
seen = set()
with open(os.path.join(OUT, 'classes', name + '.c'), 'w') as f:
    for c in ctors:
        src = func_src(c)
        f.write(src or '// missing ctor %08x\n' % c)
        for vt in re.findall(r'\*\(undefined \*\*\*\)this = &PTR_FUN_([0-9a-f]{8})', src or ''):
            entries = vtable_entries(int(vt, 16))
            f.write('\n// ---- vtable %s: %s\n' % (vt, ' '.join('%d:%08x' % (i, e) for i, e in enumerate(entries))))
            for i, e in enumerate(entries):
                if e in seen or e >= 0x45f000:   # skip framework code
                    continue
                seen.add(e)
                s = func_src(e)
                if s:
                    f.write('\n// ---- slot %d (+0x%x)\n' % (i, i * 4) + s)
print('wrote', name)
