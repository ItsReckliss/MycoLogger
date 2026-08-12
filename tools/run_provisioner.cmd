@echo off
setlocal
cd /d "%~dp0\.."
if exist ".venv\Scripts\python.exe" (
  ".venv\Scripts\python.exe" "tools\provision_transmitter.py" %*
) else (
  py "tools\provision_transmitter.py" %*
)
endlocal
