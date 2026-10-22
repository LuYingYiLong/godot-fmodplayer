@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

rem Build an optimized Windows template_debug DLL with matching PDB symbols.
call build_profile_windows.bat
if errorlevel 1 goto :failed

rem Copy the complete addon and private FMOD runtime, then refresh Godot imports.
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "tools\run_tests.ps1" -PrepareOnly -Import -Label prepare_debug %*
if errorlevel 1 goto :failed

echo Test addon prepared: tests\addons\fmod_player
popd
exit /b 0

:failed
popd
exit /b 1
