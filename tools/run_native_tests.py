"""Build and run the Capture ring-buffer and EQ preview response tests."""
from pathlib import Path
from datetime import datetime
import hashlib
import json
import os
import shlex
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'tests/native/capture_ring_buffer_test.cpp'
EQ_SOURCE = ROOT / 'tests/native/eq_preview_response_test.cpp'


def main() -> None:
    output = ROOT / 'tests/logs/runs' / ('native_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
    output.mkdir(parents=True, exist_ok=False)
    includes = [ROOT / path for path in ('src', 'godot-cpp/include', 'godot-cpp/gen/include', 'godot-cpp/gdextension')]
    commands = []
    if os.name == 'nt':
        setup = []
        if shutil.which('cl') is None:
            locator = Path(os.environ.get('ProgramFiles(x86)', 'C:/Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
            installation = subprocess.check_output([str(locator), '-latest', '-products', '*', '-requires', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'], text=True).strip()
            environment = Path(installation) / 'VC/Auxiliary/Build/vcvars64.bat'
            if not installation or not environment.is_file():
                raise SystemExit('Install MSVC C++ build tools or run from an x64 Developer Command Prompt.')
            setup = [f'call "{environment}" > "{output / "environment.log"}" 2>&1', 'if errorlevel 1 exit /b 1']
        lines = ['@echo off', 'setlocal', *setup]
        for precision in ('float', 'double'):
            executable = output / f'ring_{precision}.exe'
            command = ['cl', '/nologo', '/EHsc', '/std:c++17', '/O2', '/utf-8', '/UNDEBUG']
            command += ['/I' + str(path) for path in includes]
            if precision == 'double':
                command.append('/DREAL_T_IS_DOUBLE')
            command += [str(SOURCE), '/Fo' + str(output / f'ring_{precision}.obj'), '/Fe' + str(executable)]
            lines += [subprocess.list2cmdline(command), 'if errorlevel 1 exit /b 1', f'"{executable}"', 'if errorlevel 1 exit /b 1']
        executable = output / 'eq_preview_response.exe'
        command = ['cl', '/nologo', '/EHsc', '/std:c++17', '/O2', '/utf-8', '/UNDEBUG', '/I' + str(ROOT / 'src'), str(EQ_SOURCE), '/Fo' + str(output / 'eq_preview_response.obj'), '/Fe' + str(executable)]
        lines += [subprocess.list2cmdline(command), 'if errorlevel 1 exit /b 1', f'"{executable}"', 'if errorlevel 1 exit /b 1']
        lines.append('exit /b 0')
        batch = output / 'build.cmd'
        batch.write_text('\n'.join(lines) + '\n', encoding='utf-8', newline='\r\n')
        commands.append([os.environ.get('COMSPEC', 'cmd.exe'), '/d', '/c', str(batch)])
    else:
        compiler = shlex.split(os.environ.get('CXX', 'c++'))
        for precision in ('float', 'double'):
            executable = output / f'ring_{precision}'
            command = [*compiler, '-std=c++17', '-O2', '-UNDEBUG']
            command += ['-I' + str(path) for path in includes]
            if precision == 'double':
                command.append('-DREAL_T_IS_DOUBLE')
            commands.extend([[*command, str(SOURCE), '-o', str(executable)], [str(executable)]])
        executable = output / 'eq_preview_response'
        commands.extend([[*compiler, '-std=c++17', '-O2', '-UNDEBUG', '-I' + str(ROOT / 'src'), str(EQ_SOURCE), '-o', str(executable)], [str(executable)]])
    log_path = output / 'native_tests.log'
    with log_path.open('w', encoding='utf-8') as log:
        for command in commands:
            completed = subprocess.run(command, cwd=output, stdout=log, stderr=subprocess.STDOUT)
            if completed.returncode:
                raise SystemExit(f'Native tests failed; see {log_path}')
    (output / 'summary.json').write_text(json.dumps({
        'passed': True,
        'variants': ['float', 'double'],
        'source_sha256': hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
        'eq_response_source_sha256': hashlib.sha256(EQ_SOURCE.read_bytes()).hexdigest(),
    }, indent=2) + '\n', encoding='utf-8', newline='\n')
    print(log_path.read_text(encoding='utf-8', errors='replace').strip())
    print(f'Test output: {output}')


if __name__ == '__main__':
    main()
