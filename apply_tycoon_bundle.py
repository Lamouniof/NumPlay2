#!/usr/bin/env python3
"""Met NumTycoon DANS NumPlay.nwa et releve la limite d'espace des applications a 6 Mo.
Modifie vos fichiers sur place (rien n'est remplace). A lancer a la racine du depot :
    python3 apply_tycoon_bundle.py
Idempotent : vous pouvez le relancer."""
import re, sys, json

LIMIT = 6 * 1024 * 1024          # 6291456 octets
FLASH_END = 0x90200000 + LIMIT   # 0x90800000

def rw(path, fn):
    s = open(path, encoding="utf-8").read()
    n = fn(s)
    if n != s:
        open(path, "w", encoding="utf-8", newline="\n").write(n)
        print("modifie :", path)
    else:
        print("deja bon :", path)

def makefile(s):
    s2 = re.sub(r"^APP_SPACE = \d+\s*$", f"APP_SPACE = {LIMIT}", s, flags=re.M)
    s2 = re.sub(r"^# the calculator's space for installed apps.*$",
                "# the space for installed apps: 6 MB with the memory-extension system (stock calculator: 2031616)",
                s2, flags=re.M)
    if "APP_SPACE = " not in s2:
        sys.exit("APP_SPACE introuvable dans le Makefile")
    return s2

def emu(s):
    s2 = re.sub(r"^EXT_FLASH_END = 0x[0-9A-Fa-f]+", f"EXT_FLASH_END = {hex(FLASH_END)}", s, flags=re.M)
    if "EXT_FLASH_END = " not in s2:
        sys.exit("EXT_FLASH_END introuvable dans tools/emu.py")
    return s2

ENTRY = '''  {
    "id": "tycoon",
    "title": "NumTycoon",
    "tagline": "Build your empire",
    "credit": null,
    "top": "0x2F6FD6",
    "bottom": "0x0E1330",
    "accent": "0xFFC933",
    "records": [
      "tycoon.sav"
    ],
    "shots": [
      "docs/shot.png"
    ],
    "app": "NumTycoon.nwa",
    "repo": "https://github.com/Mason363/NumPlay/tree/main/games/tycoon"
  },
'''

def games(s):
    if '"id": "tycoon"' in s:
        return s
    m = re.search(r'^  \{\n    "id": "numvisuals"', s, re.M)
    if not m:
        sys.exit('bloc "numvisuals" introuvable dans games/games.json')
    out = s[:m.start()] + ENTRY + s[m.start():]
    json.loads(out)
    return out

def release(s):
    return re.sub(r'    "tycoon": \(.*?\),\n(?=    "|\})', "", s, flags=re.S)

rw("Makefile", makefile)
rw("tools/emu.py", emu)
rw("games/games.json", games)
rw("tools/release.py", release)
print("OK : NumTycoon est dans le bundle, limite = %d octets." % LIMIT)
