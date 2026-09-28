#!/usr/bin/env python3
"""Extract/check the bonus mapping of the supplied DOS ke.exe (no game execution).

Uses only Python's standard library. --verify also requires a C++20 compiler.
The addresses and effect meanings are specific to the executable fingerprint below.
See ../docs/bonuses.md for the handler traces behind the names.
"""
import argparse
import collections
import hashlib
from pathlib import Path
import re
import struct
import subprocess
import tempfile

EXE_SHA256 = '5ee2470f159f7705d26469dcba4b0f153c5724c072acc7133af67a536c651849'
# Runtime ID order, independently traced from the dispatch table at 0x49304.
EFFECTS = [
    ('enlarge_paddle', 0x2DE00), ('damage_paddle', 0x2D7E8),
    ('score_multiplier', 0x2D838), ('reverse_controls', 0x2D860),
    ('shrink_balls', 0x2D908), ('glue_paddle', 0x2D938),
    ('extra_life', 0x2D96C), ('extra_ball', 0x2D994),
    ('enlarge_balls', 0x2D910), ('darkness', 0x2DA4C),
    ('speed_up_all_balls', 0x2DAF8), ('slow_all_balls', 0x2DAF0),
    ('autopilot', 0x2DB08), ('flying_paddle', 0x2DBA0),
    ('freeze_paddle', 0x2DBDC), ('shield', 0x2DC14),
    ('cannon', 0x2DC54), ('power_ball', 0x2DC94),
    ('ghost_balls', 0x2DCB4), ('extra_paddle', 0x2DD34),
    ('rapid_cannon', 0x2DD60), ('random', 0x2DDA0),
    ('shrink_paddle', 0x2DDF8), ('single_gun', 0x2DE20),
    ('double_gun', 0x2DE60), ('rapid_single_gun', 0x2DEA0),
    ('rapid_double_gun', 0x2DEE0), ('clear_enemies', 0x2DF20),
]


class Executable:
    """Read initialized bytes through the LE object/page tables (unrelocated file)."""
    def __init__(self, path):
        self.data = path.read_bytes()
        digest = hashlib.sha256(self.data).hexdigest()
        if digest != EXE_SHA256:
            raise ValueError(f'Unrecognized ke.exe SHA-256: {digest}; addresses need re-analysis')
        self.le = struct.unpack_from('<I', self.data, 0x3C)[0]
        if self.data[self.le:self.le + 2] != b'LE':
            raise ValueError('Not an LE executable')
        self.page_size = self.u32(self.le + 0x28)
        self.page_table = self.le + self.u32(self.le + 0x48)
        self.page_data = self.u32(self.le + 0x80)
        obj_table = self.le + self.u32(self.le + 0x40)
        self.objects = [struct.unpack_from('<6I', self.data, obj_table + i * 24)
                        for i in range(self.u32(self.le + 0x44))]

    def u32(self, offset):
        return struct.unpack_from('<I', self.data, offset)[0]

    def read(self, address, size):
        for length, base, _, first_page, page_count, _ in self.objects:
            if base <= address and address + size <= base + length:
                out = bytearray()
                while size:
                    rel = address - base
                    page, within = divmod(rel, self.page_size)
                    if page >= page_count:
                        raise ValueError('Read of an uninitialized page')
                    entry = self.page_table + (first_page - 1 + page) * 4
                    physical = int.from_bytes(self.data[entry:entry + 3], 'big')
                    if self.data[entry + 3] != 0 or physical == 0:
                        raise ValueError('Unsupported LE page flags')
                    off = self.page_data + (physical - 1) * self.page_size + within
                    n = min(size, self.page_size - within)
                    out.extend(self.data[off:off + n])
                    address += n
                    size -= n
                return bytes(out)
        raise ValueError(f'Address outside LE objects: {address:#x}')

    def pointer(self, address, target_object):
        return struct.unpack('<I', self.read(address, 4))[0] + self.objects[target_object - 1][1]

    def animation(self, address):
        frames, ticks = [], []
        for index in range(32):
            frame, control = struct.unpack('<Ih', self.read(address + index * 6, 6))
            if frame == 0:
                if control != -len(frames):
                    raise ValueError(f'Unexpected animation loop at {address:#x}')
                return frames, ticks
            frames.append(frame - 1)
            ticks.append(control)
        raise ValueError('Unterminated animation')


def require(condition, message):
    if not condition:
        raise ValueError(message)


def verify_source(exe, animations, source, compiler):
    # Check the actual x86 operations behind the oracle, including range-check order.
    require(exe.read(0x329BA, 15).hex() == '8a03c0e8020fb6c04850e837adffff',
            'destroy_brick no longer contains shift/subtract/call sequence')
    require(exe.read(0x2D70D, 10).hex() == '0fb65c240883fb1c7d61',
            'spawn_bonus no longer rejects runtime ID >= 28 before masking')
    require(exe.read(0x2D5D7, 11).hex() == '660fb6500a8a400b42241f',
            'catch handler magnitude/ID decoding changed')
    cells = (source / 'resources/cell.hh').read_text()
    for index, (name, _) in enumerate(EFFECTS):
        require(re.search(r'\b' + name + r'\s*=\s*' + str(index) + r'\s*,', cells),
                f'Wrong enum identity: {name} must be {index}')
    sprites = (source / 'assets/sprites.hh').read_text()
    table = sprites.split('ke_spell_capsule_anim =', 1)[1].split('};', 1)[0]
    entries = re.findall(r'\{\{([\d,\s]+)\},\s*(\d+),\s*(\d+)\}', table)
    require(len(entries) == 28, 'Expected 28 source animations')
    for index, (raw_frames, count, ticks) in enumerate(entries):
        frames = [int(n) for n in raw_frames.split(',') if n.strip()]
        require((frames, [int(ticks)] * int(count)) == animations[index],
                f'Animation mismatch for {EFFECTS[index][0]}')

    # Execute the production constexpr decoder for all 65,536 byte pairs. The
    # oracle separately follows the original call boundary's uint8 truncation.
    with tempfile.TemporaryDirectory(prefix='ke-bonus-check-') as tmp:
        cpp = Path(tmp) / 'probe.cc'
        binary = Path(tmp) / 'probe'
        cpp.write_text('''#include <ke/resources/cell.hh>
#include <cstdio>
int main() {
    for (int tile = 0; tile < 256; ++tile)
        for (int attr = 0; attr < 256; ++attr) {
            const auto c = rs::ke_cell::decode(tile, attr);
            std::putchar(static_cast<unsigned char>(c.bonus_type));
            std::putchar(c.bonus_mag);
            std::putchar(c.drops_bonus);
        }
}
''')
        subprocess.run([compiler, '-std=c++20', '-I', str(source.parent),
                        str(cpp), str(source / 'resources/cell.cc'), '-o', str(binary)], check=True)
        actual = subprocess.check_output([str(binary)])
    expected = bytearray()
    for tile in range(256):
        for attr in range(256):
            candidate = ((attr >> 2) - 1) & 0xFF
            drops = 0x31 <= tile <= 0x60 and (attr & 0xFC) != 0 and candidate < 28
            expected.extend((candidate & 31 if drops else 255, (attr & 3) + 1 if drops else 0, drops))
    require(actual == expected, 'Production decoder differs from the executable oracle')
    print('Verified: 28 identities, 28 animations, 65,536 tile/attribute decodes.')


def inspect_levels(path):
    data = path.read_bytes()
    count, header, _, names, _, payload, _ = struct.unpack_from('<7I', data)
    totals = collections.Counter()
    level_count = 0
    for i in range(count):
        name_off, _, rel, size = struct.unpack_from('<HHII', data, header + i * 12)
        end = data.index(0, names + name_off)
        name = data[names + name_off:end].decode('ascii')
        if not name.lower().endswith('.tab'):
            continue
        require(size % 586 == 0, f'Partial level in {name}')
        levels = size // 586
        level_count += levels
        for level in range(levels):
            start = payload + rel + 586 * level + 10
            for cell in range(288):
                attr, tile = data[start + cell * 2:start + cell * 2 + 2]
                if 0x31 <= tile <= 0x60 and 1 <= attr >> 2 <= 28:
                    totals[(attr >> 2) - 1] += 1
    print(f'Archive: {level_count} levels; {sum(totals.values())} bonus-bearing bricks; '
          f'{len(totals)} distinct effects.')
    print('Per-ID counts:', ', '.join(f'{i}:{totals[i]}' for i in range(28)))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('exe', type=Path)
    parser.add_argument('--rsc', type=Path)
    parser.add_argument('--verify', action='store_true', help='Check production enum, animations and decoder')
    parser.add_argument('--cxx', default='c++')
    args = parser.parse_args()
    exe = Executable(args.exe)
    animations = []
    print(' ID  TAB attr   handler   KE_SPELL frames / ticks   effect')
    for i, (name, handler) in enumerate(EFFECTS):
        require(exe.pointer(0x49304 + i * 4, 1) == handler, f'Handler mismatch for ID {i}')
        frames, ticks = exe.animation(exe.pointer(0x49284 + i * 4, 2))
        animations.append((frames, ticks))
        print(f'{i:3}  {4*(i+1):02X}..{4*(i+1)+3:02X}     {handler:05X}    '
              f'{",".join(map(str, frames))} / {ticks[0]}   {name}')
    if args.verify:
        verify_source(exe, animations, Path(__file__).resolve().parents[1], args.cxx)
    if args.rsc:
        inspect_levels(args.rsc)


if __name__ == '__main__':
    main()
