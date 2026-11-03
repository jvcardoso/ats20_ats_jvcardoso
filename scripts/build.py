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

ARTIFACTS = (
    'ATS_EX.ino.hex',
    'ATS_EX.ino.eep',
    'ATS_EX.ino.elf',
    'ATS_EX.ino.with_bootloader.hex',
    'ATS_EX.ino.with_bootloader.bin',
)

def main():
    args = parser.parse_args()
    cli = args.cli or shutil.which('arduino-cli')
    bundled = Path('/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli')
    if not cli and bundled.is_file():
        cli = str(bundled)
    cli_error = None
    if not cli or (not Path(cli).is_file() and not shutil.which(cli)):
        cli_error = 'Install Arduino CLI or pass a valid path with --cli.'
    profiles = ['nano', 'uno'] if args.profile == 'all' else [args.profile]
    variants = ['1', '0'] if args.rds == 'all' else [args.rds]
    failed = False
    for profile in profiles:
        for rds in variants:
            output = args.output.resolve() / f'{profile}-rds{rds}'
            output.mkdir(parents=True, exist_ok=True)
            for name in ARTIFACTS:
                stale = output / name
                if stale.exists():
                    stale.unlink()
            if cli_error:
                (output / 'compile.log').write_text(cli_error)
                print(f'{output}: {cli_error}', file=sys.stderr)
                failed = True
                continue
            base = [cli, '--no-color']
            if args.config_file:
                base += ['--config-file', str(args.config_file.resolve())]
            command = base + ['compile', '--profile', profile, '--warnings', 'all',
                              '--build-property', f'compiler.cpp.extra_flags=-DUSE_RDS={rds}',
                              '--build-path', str(output / 'objects'),
                              '--output-dir', str(output), str(ROOT / 'ATS_EX')]
            print(f'Compiling {profile}, USE_RDS={rds}...', flush=True)
            try:
                result = subprocess.run(command, capture_output=True, text=True, timeout=900)
            except (subprocess.TimeoutExpired, OSError) as exc:
                message = f'Compile failed to run: {exc}'
                (output / 'compile.log').write_text(message)
                print(message, file=sys.stderr)
                failed = True
                continue
            log = '\n'.join(command) + f'\n\nExit code: {result.returncode}\n\n' + result.stdout + result.stderr
            (output / 'compile.log').write_text(log)
            for line in result.stdout.splitlines():
                if line.startswith(('Sketch uses', 'Global variables')):
                    print(line, flush=True)
            if result.returncode:
                print(result.stderr[-4000:], file=sys.stderr)
                failed = True
            elif not (output / 'ATS_EX.ino.hex').is_file():
                print('Compile succeeded but produced no HEX file.', file=sys.stderr)
                failed = True
            print(f'Log: {output / "compile.log"}', flush=True)
    return int(failed)

if __name__ == '__main__':
    sys.exit(main())
