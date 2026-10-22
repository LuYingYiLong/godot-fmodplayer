@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

rem Keep optimized code and distinct function addresses for CPU profiling.
call scons platform=windows target=template_debug arch=x86_64 optimize=speed_trace debug_symbols=yes -j16
if errorlevel 1 goto :failed

popd
exit /b 0

:failed
popd
exit /b 1
