@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

rem Keep optimized code and distinct function addresses for CPU profiling.
if not exist "tests\logs\builds" mkdir "tests\logs\builds"
call scons platform=windows target=template_debug arch=x86_64 optimize=speed_trace debug_symbols=yes -j16 > "tests\logs\builds\build_profile.log" 2>&1
if errorlevel 1 goto :failed

echo Build completed. Log: tests\logs\builds\build_profile.log
popd
exit /b 0

:failed
type "tests\logs\builds\build_profile.log"
popd
exit /b 1
