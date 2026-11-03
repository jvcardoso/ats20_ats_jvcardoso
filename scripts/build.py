#!/usr/bin/env python3
"""Compile pinned Nano/Uno profiles with and without RDS; save logs and binaries."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--cli', help='Path to arduino-cli')
parser.add_argument('--config-file', type=Path, help='Optional Arduino CLI configuration')
parser.add_argument('--profile', choices=['nano', 'uno', 'all'], default='all')
parser.add_argument('--rds', choices=['0', '1', 'all'], default='all')
parser.add_argument('--output', type=Path, default=ROOT / 'build')

def main():
    args = parser.parse_args()
    cli = args.cli or shutil.which('arduino-cli')
    bundled = Path('/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli')
    if not cli and bundled.is_file():
        cli = str(bundled)
    if not cli:
        parser.error('Install Arduino CLI or pass its path with --cli.')
    base = [cli, '--no-color']
    if args.config_file:
        base += ['--config-file', str(args.config_file.resolve())]
    profiles = ['nano', 'uno'] if args.profile == 'all' else [args.profile]
    variants = ['1', '0'] if args.rds == 'all' else [args.rds]
    failed = False
    for profile in profiles:
        for rds in variants:
            output = args.output.resolve() / f'{profile}-rds{rds}'
            output.mkdir(parents=True, exist_ok=True)
            command = base + ['compile', '--profile', profile, '--warnings', 'all',
                              '--build-property', f'compiler.cpp.extra_flags=-DUSE_RDS={rds}',
                              '--build-path', str(output / 'objects'),
                              '--output-dir', str(output), str(ROOT / 'ATS_EX')]
            print(f'Compiling {profile}, USE_RDS={rds}...', flush=True)
            result = subprocess.run(command, capture_output=True, text=True)
            log = '\n'.join(command) + f'\n\nExit code: {result.returncode}\n\n' + result.stdout + result.stderr
            (output / 'compile.log').write_text(log)
            for line in result.stdout.splitlines():
                if line.startswith(('Sketch uses', 'Global variables')):
                    print(line, flush=True)
            if result.returncode:
                print(result.stderr[-4000:], file=sys.stderr)
                failed = True
            print(f'Log: {output / "compile.log"}', flush=True)
    return int(failed)

if __name__ == '__main__':
    sys.exit(main())
