#!/usr/bin/env python3
"""Annotates re/ghidra-out/decomp.c with names from the executable's own tables.

Image pointers live in an array at <game object> + 0x688 (157 entries, table order of
the image table at VA 0x527820), so "+ 0x6cc" gets the comment /*img:tank*/.
Writes re/ghidra-out/decomp_named.c and funcs/ (one file per function).
"""
import re, struct, os, sys

ROOT = os.path.join(os.path.dirname(__file__), '..', '..')
EXE = os.path.join(ROOT, 're', 'inner_game.exe')
OUT = os.path.join(ROOT, 're', 'ghidra-out')

data = open(EXE, 'rb').read()
images = [data[0x127820 + i * 52:0x127820 + i * 52 + 40].split(b'\0')[0].decode() for i in range(157)]
IMG_BASE = 0x688
img_by_off = {IMG_BASE + 4 * i: n.replace('\\', '/') for i, n in enumerate(images)}

src = open(os.path.join(OUT, 'decomp.c')).read()

named = re.sub(r'(\+ )0x([0-9a-f]{3})\b(\))', lambda m: m.group(0)[:-1] + (f' /*img:{img_by_off[int(m.group(2),16)]}*/' if int(m.group(2),16) in img_by_off else '') + ')', src)
# Sound IDs: index into the sound table at VA 0x529808 (24-byte records: char[16], double).
sounds = []
for i in range(92):
    sounds.append(data[0x129808 + i * 24:0x129808 + i * 24 + 16].split(b'\0')[0].decode())

def snd(m):
    v = int(m.group(2), 0)
    return m.group(0) + (f' /*snd:{sounds[v]}*/' if 0 <= v < len(sounds) else '')

# App_PlaySample(app, id) / GetSoundInstance(id) via SoundManager vtable +0x1c
named = re.sub(r'(App_PlaySample(?:Pan)?\([^,()]*(?:\([^()]*\))?[^,()]*,\s*)(0x[0-9a-f]+|\d+)', snd, named)
named = re.sub(r'(\+ 0x438\) \+ 0x1c\)\)\()(0x[0-9a-f]+|\d+)', snd, named)
# Game PlaySound(id, pan): app vtable slot +0xb8 (FUN_00401150, per-sound cooldown)
named = re.sub(r'(\+ 0xb8\)\)\()(0x[0-9a-f]+|\d+)', snd, named)
open(os.path.join(OUT, 'decomp_named.c'), 'w').write(named)

fdir = os.path.join(OUT, 'funcs')
os.makedirs(fdir, exist_ok=True)
for chunk in named.split('// ==== ')[1:]:
    head = chunk.split('\n', 1)[0]
    addr = head.split('@ ')[1].split(' ')[0]
    open(os.path.join(fdir, addr + '.c'), 'w').write('// ==== ' + chunk)
print('images annotated:', named.count('/*img:'), 'sounds:', named.count('/*snd:'), 'functions:', len(os.listdir(fdir)))
