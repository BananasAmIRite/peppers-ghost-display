@echo off
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%app\rpi\src"
start "" /b "%SCRIPT_DIR%app\rpi\env\Scripts\pythonw.exe" -m windows.main
exit