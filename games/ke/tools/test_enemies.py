#!/usr/bin/env python3
"""Build/run the real-resource enemy integration test using an existing Ninja build.

No CMake reconfiguration or writes to the IDE build tree. Objects/executable are
created in a temporary directory. Set KE_TEST_RSC to use another resource path;
KE_TEST_SCREENSHOT optionally saves the rendered eight-enemy frame as a BMP.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=root.parents[1] / 'cmake-build-debug')
    args = parser.parse_args()
    build = args.build_dir.resolve()
    entries = [e for e in json.loads((build / 'compile_commands.json').read_text())
               if '/games/ke/' in e['file'] and 'CMakeFiles/ke.dir/' in e['command']]
    if not entries:
        raise RuntimeError('No configured ke compile commands found')
    with tempfile.TemporaryDirectory(prefix='ke-enemy-tests-') as tmp:
        out = Path(tmp)
        def compile_one(entry):
            command = shlex.split(entry['command'])
            index = command.index('-o') + 1
            original = command[index]
            output = out / (Path(entry['file']).stem + '.o')
            command[index] = str(output)
            if Path(entry['file']).name == 'krypton_egg.cc':
                command[command.index(entry['file'])] = str(root / 'tests/enemies.cc')
            # A compilation database may contain dependency-output flags.
            clean = []
            it = iter(command)
            for arg in it:
                if arg in ('-MF', '-MT', '-MQ'):
                    next(it)
                elif arg not in ('-MD', '-MMD', '-MP'):
                    clean.append(arg)
            result = subprocess.run(clean, cwd=build, capture_output=True, text=True)
            if result.returncode:
                raise RuntimeError(result.stdout + result.stderr)
            return original, str(output)
        with ThreadPoolExecutor(max_workers=2) as pool:
            outputs = dict(pool.map(compile_one, entries))
        link = subprocess.check_output(['ninja', '-C', str(build), '-t', 'commands', 'ke'], text=True).splitlines()[-1]
        command = shlex.split(link.removeprefix(': && ').removesuffix(' && :'))
        command = [outputs.get(arg, arg) for arg in command if not arg.startswith('-Wl,--dependency-file=')]
        binary = out / 'enemy_tests'
        command[command.index('-o') + 1] = str(binary)
        subprocess.run(command, cwd=build, check=True)
        env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
        subprocess.run([str(binary)], env=env, check=True, timeout=60)


if __name__ == '__main__':
    main()
