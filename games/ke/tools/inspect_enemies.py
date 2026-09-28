#!/usr/bin/env python3
"""Extract enemy animations from the fingerprinted DOS executable and check C++ data.

Usage: python3 tools/inspect_enemies.py ~/games/ke/Krypton-Egg_DOS_EN/ke.exe
Uses the standard library and the LE reader in inspect_bonuses.py.
"""
import argparse
from pathlib import Path
import re
import sys

sys.dont_write_bytecode = True
from inspect_bonuses import Executable, require


def source_animations(text):
    entries = re.findall(r'\{\s*\{([\d,\s]+)\},\s*(\d+),\s*(\d+)\s*\}', text)
    return [([int(n) for n in frames.split(',') if n.strip()], [int(ticks)] * int(count))
            for frames, count, ticks in entries]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    args = parser.parse_args()
    exe = Executable(args.exe)
    source = (Path(__file__).resolve().parents[1] / 'assets/sprites.hh').read_text()
    table = source.split('ke_nmy_enemy_anim =', 1)[1].split('};', 1)[0]
    animations = source_animations(table)
    require(len(animations) == 8, 'Expected eight source enemy animations')
    for index, actual in enumerate(animations):
        address = exe.pointer(0x496B8 + index * 4, 2)
        require(actual == exe.animation(address), f'Enemy {index} animation differs from DOS')
        print(f'Type {index}, {address:#x}: frames {actual[0]}, {actual[1][0]} ticks/frame')
    for name, address in [('enemy_hatch_anim', 0x49384), ('enemy_death_anim', 0x49634),
                          ('paddle_death_anim', 0x48EEC)]:
        declaration = source.split(name + ' =', 1)[1].split(';', 1)[0]
        require(source_animations(declaration) == [exe.animation(address)],
                f'{name} differs from DOS')
    print('Verified: eight enemy types, hatch, enemy death and paddle death animations.')


if __name__ == '__main__':
    main()
