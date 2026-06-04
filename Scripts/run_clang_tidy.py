#!/usr/bin/env python3
"""Run clang-tidy over Fleur first-party C++ sources.

Decoupled from the normal Visual Studio build: clang-tidy needs a
compile_commands.json, which the VS generator does not emit, so this script
configures a throwaway Ninja build (Build/tidy) purely to produce that database,
then runs clang-tidy on the first-party translation units only.

Third-party code under */External/ is never analysed. Header diagnostics are
restricted to first-party headers by HeaderFilterRegex in the repo-root
.clang-tidy.

Tooling is auto-discovered (no need to be inside a VS developer shell):
  - clang-tidy / ninja  -> Visual Studio 2022 install (or PATH / standalone LLVM)
  - cl, INCLUDE, etc.   -> captured from vcvars64.bat

Usage:
  python Scripts/run_clang_tidy.py                 # analyse all first-party TUs
  python Scripts/run_clang_tidy.py --reconfigure   # force re-run of CMake configure
  python Scripts/run_clang_tidy.py --strict        # exit nonzero on any finding (CI)
  python Scripts/run_clang_tidy.py --jobs 8        # parallelism (default: CPU count)
  python Scripts/run_clang_tidy.py path\\to\\file.cpp ...   # only the given files
"""
from __future__ import annotations

import argparse
import json
import multiprocessing
import os
import shlex
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
TIDY_BUILD_DIR = ROOT / 'Build' / 'tidy'

# Top-level directories whose translation units we analyse. Engine is special-cased
# to Engine/Fleur so that Engine/External/ is excluded.
FIRST_PARTY_TOP = {'MemoryManager', 'CoreLib', 'Tessera', 'Sandbox'}

PROGRAM_FILES = os.environ.get('ProgramFiles', r'C:\Program Files')
PROGRAM_FILES_X86 = os.environ.get(
    'ProgramFiles(x86)', r'C:\Program Files (x86)',
)
VS_EDITIONS = ('Community', 'Professional', 'Enterprise', 'Preview')


def fail(message: str) -> None:
    print(f'[run_clang_tidy] ERROR: {message}', file=sys.stderr)
    sys.exit(1)


def find_first(candidates: list[Path]) -> Path | None:
    for path in candidates:
        if path.is_file():
            return path
    return None


def find_clang_tidy() -> Path:
    candidates = [
        Path(PROGRAM_FILES) / 'Microsoft Visual Studio' / '2022' / edition
        / 'VC' / 'Tools' / 'Llvm' / 'x64' / 'bin' / 'clang-tidy.exe'
        for edition in VS_EDITIONS
    ]
    candidates.append(Path(PROGRAM_FILES) / 'LLVM' / 'bin' / 'clang-tidy.exe')
    found = find_first(candidates)
    if found:
        return found
    on_path = _which('clang-tidy')
    if on_path:
        return on_path
    fail('clang-tidy not found (looked in VS 2022 LLVM, standalone LLVM, PATH)')
    raise AssertionError  # unreachable, keeps type checkers happy


def find_ninja() -> Path:
    candidates = [
        Path(PROGRAM_FILES) / 'Microsoft Visual Studio' / '2022' / edition
        / 'Common7' / 'IDE' / 'CommonExtensions' / 'Microsoft' / 'CMake'
        / 'Ninja' / 'ninja.exe'
        for edition in VS_EDITIONS
    ]
    found = find_first(candidates)
    if found:
        return found
    on_path = _which('ninja')
    if on_path:
        return on_path
    fail('ninja not found (looked in VS 2022 CMake bundle, PATH)')
    raise AssertionError


def find_vcvars() -> Path:
    vswhere = Path(PROGRAM_FILES_X86) / 'Microsoft Visual Studio' / \
        'Installer' / 'vswhere.exe'
    if vswhere.is_file():
        out = subprocess.run(
            [
                str(vswhere), '-latest', '-products', '*',
                '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
                '-property', 'installationPath',
            ],
            capture_output=True, text=True,
        ).stdout.strip()
        if out:
            vcvars = Path(out) / 'VC' / 'Auxiliary' / 'Build' / 'vcvars64.bat'
            if vcvars.is_file():
                return vcvars
    # Fallback: probe known edition paths directly.
    candidates = [
        Path(PROGRAM_FILES) / 'Microsoft Visual Studio' / '2022' / edition
        / 'VC' / 'Auxiliary' / 'Build' / 'vcvars64.bat'
        for edition in VS_EDITIONS
    ]
    found = find_first(candidates)
    if found:
        return found
    fail('vcvars64.bat not found (need MSVC build tools installed)')
    raise AssertionError


def _which(name: str) -> Path | None:
    from shutil import which
    resolved = which(name)
    return Path(resolved) if resolved else None


def capture_vcvars_env(vcvars: Path) -> dict[str, str]:
    """Run vcvars64.bat and capture the resulting environment.

    clang-tidy resolves MSVC system headers via INCLUDE, and the Ninja configure
    needs cl on PATH, so every subprocess runs with this environment.
    """
    marker = '__VCVARS_ENV_BELOW__'
    # shell=True so cmd.exe parses the quoted path natively. Passing a list would
    # make Python escape the inner quotes as \" and break the `call`.
    completed = subprocess.run(
        f'call "{vcvars}" >nul 2>&1 && echo {marker} && set',
        shell=True, capture_output=True, text=True,
    )
    if completed.returncode != 0:
        fail(f'vcvars64.bat failed:\n{completed.stdout}\n{completed.stderr}')
    env: dict[str, str] = {}
    seen_marker = False
    for line in completed.stdout.splitlines():
        if not seen_marker:
            if line.strip() == marker:
                seen_marker = True
            continue
        if '=' in line:
            key, _, value = line.partition('=')
            env[key] = value
    if not env:
        fail('failed to capture environment from vcvars64.bat')
    return env


def configure(env: dict[str, str], ninja: Path, reconfigure: bool) -> None:
    compile_db = TIDY_BUILD_DIR / 'compile_commands.json'
    if compile_db.is_file() and not reconfigure:
        print(f'[run_clang_tidy] reusing {compile_db}')
        strip_pch_flags(compile_db)
        return
    TIDY_BUILD_DIR.mkdir(parents=True, exist_ok=True)
    print('[run_clang_tidy] configuring throwaway Ninja build for '
          'compile_commands.json ...')
    cmd = [
        'cmake', '-S', str(ROOT), '-B', str(TIDY_BUILD_DIR),
        '-G', 'Ninja',
        f'-DCMAKE_MAKE_PROGRAM={ninja}',
        '-DCMAKE_BUILD_TYPE=Debug',
        '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
        '-DFLEUR_PLATFORM=x64',
        '-DENABLE_FLEUR_TEST=OFF',
        '-DFLEUR_LIB_TYPE=STATIC',
    ]
    completed = subprocess.run(cmd, env=env)
    if completed.returncode != 0:
        fail('CMake configure failed (see output above)')
    if not compile_db.is_file():
        fail(f'configure finished but {compile_db} was not produced')
    strip_pch_flags(compile_db)


def strip_pch_flags(compile_db: Path) -> None:
    """Remove MSVC binary-PCH flags so clang-tidy parses cleanly.

    CMake's target_precompile_headers emits /Yu (use), /Yc (create) and /Fp
    (binary .pch path) alongside /FI (forced include of the generated wrapper
    header). clang-tidy cannot read the MSVC .pch and we never build it, so it
    errors out. Dropping /Yu /Yc /Fp while keeping /FI makes the wrapper header
    get included textually instead — includes still resolve, no PCH needed.
    """
    entries = json.loads(compile_db.read_text(encoding='utf-8'))
    drop_prefixes = ('/Yu', '/Yc', '/Fp', '-Yu', '-Yc', '-Fp')
    changed = False
    for entry in entries:
        if 'command' in entry:
            tokens = shlex.split(entry['command'], posix=False)
            kept = [t for t in tokens if not t.startswith(drop_prefixes)]
            if len(kept) != len(tokens):
                entry['command'] = subprocess.list2cmdline(kept)
                changed = True
        elif 'arguments' in entry:
            args = entry['arguments']
            kept = [t for t in args if not t.startswith(drop_prefixes)]
            if len(kept) != len(args):
                entry['arguments'] = kept
                changed = True
    if changed:
        compile_db.write_text(json.dumps(entries, indent=2), encoding='utf-8')
        print('[run_clang_tidy] stripped MSVC PCH flags from compile_commands.json')


def is_first_party(source: Path) -> bool:
    try:
        rel = source.resolve().relative_to(ROOT)
    except ValueError:
        return False
    parts = rel.parts
    if 'External' in parts:
        return False
    if parts and parts[0] in FIRST_PARTY_TOP:
        return True
    return len(parts) >= 2 and parts[0] == 'Engine' and parts[1] == 'Fleur'


def select_sources(only: list[str]) -> list[Path]:
    compile_db = TIDY_BUILD_DIR / 'compile_commands.json'
    entries = json.loads(compile_db.read_text(encoding='utf-8'))
    db_files = {Path(e['file']).resolve() for e in entries}

    if only:
        wanted = {Path(f).resolve() for f in only}
        selected = sorted(db_files & wanted)
        missing = wanted - db_files
        for path in sorted(missing):
            print(f'[run_clang_tidy] skip (not a known TU): {path}')
        return selected

    return sorted(f for f in db_files if is_first_party(f))


def run_one(args: tuple[Path, Path, bool, dict[str, str]]) -> tuple[Path, int, str]:
    clang_tidy, source, strict, env = args
    cmd = [str(clang_tidy), '-p', str(TIDY_BUILD_DIR), '--quiet']
    if strict:
        cmd.append('--warnings-as-errors=*')
    cmd.append(str(source))
    completed = subprocess.run(cmd, env=env, capture_output=True, text=True)
    output = (completed.stdout or '') + (completed.stderr or '')
    return source, completed.returncode, output


def main() -> int:
    if os.name != 'nt':
        fail('this script targets Windows / MSVC only')

    parser = argparse.ArgumentParser(description='Run clang-tidy on Fleur sources')
    parser.add_argument('--reconfigure', action='store_true',
                        help='force CMake re-configure of the tidy build')
    parser.add_argument('--strict', action='store_true',
                        help='treat any finding as an error (exit nonzero)')
    parser.add_argument('--jobs', type=int, default=multiprocessing.cpu_count(),
                        help='number of parallel clang-tidy processes')
    parser.add_argument('files', nargs='*',
                        help='specific source files (default: all first-party TUs)')
    opts = parser.parse_args()

    clang_tidy = find_clang_tidy()
    ninja = find_ninja()
    vcvars = find_vcvars()
    print(f'[run_clang_tidy] clang-tidy: {clang_tidy}')

    env = capture_vcvars_env(vcvars)
    configure(env, ninja, opts.reconfigure)

    sources = select_sources(opts.files)
    if not sources:
        print('[run_clang_tidy] no first-party sources matched; nothing to do')
        return 0
    print(f'[run_clang_tidy] analysing {len(sources)} file(s) '
          f'with {opts.jobs} job(s) ...')

    tasks = [(clang_tidy, src, opts.strict, env) for src in sources]
    flagged: list[Path] = []
    with ThreadPoolExecutor(max_workers=opts.jobs) as pool:
        for source, code, output in pool.map(run_one, tasks):
            text = output.strip()
            if text:
                print(f'\n===== {source.relative_to(ROOT)} =====')
                print(text)
            if code != 0:
                flagged.append(source)

    print('\n[run_clang_tidy] -------- summary --------')
    print(f'[run_clang_tidy] files analysed : {len(sources)}')
    print(f'[run_clang_tidy] files w/ issues: {len(flagged)}')
    for path in flagged:
        print(f'  - {path.relative_to(ROOT)}')

    if opts.strict and flagged:
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
