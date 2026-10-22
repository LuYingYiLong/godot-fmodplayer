@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

rem Extra SCons options can override custom.py, for example arch=x86_64.
call scons platform=android target=template_debug arch=arm64 -j16 %*
if errorlevel 1 goto :failed
call scons platform=android target=template_release arch=arm64 -j16 %*
if errorlevel 1 goto :failed

popd
exit /b 0

:failed
popd
exit /b 1
