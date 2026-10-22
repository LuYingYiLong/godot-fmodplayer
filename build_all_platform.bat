@echo off
setlocal
pushd "%~dp0"
if errorlevel 1 exit /b 1

call scons platform=windows target=template_debug -j16
if errorlevel 1 goto :failed
call scons platform=windows target=template_release -j16
if errorlevel 1 goto :failed
call build_android.bat
if errorlevel 1 goto :failed

popd
exit /b 0

:failed
popd
exit /b 1
