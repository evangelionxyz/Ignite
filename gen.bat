@echo off
>nul 2>&1 "%SYSTEMROOT%\system32\cacls.exe" "%SYSTEMROOT%\system32\config\system"

rem
if not exist "%~dp0\Scripts\__pycache__" (
    if %errorlevel% NEQ 0 (
        powershell -Command "Start-Process -FilePath '%0' -Verb RunAs"
        exit /b
    )

    python -m pip install requests
    python -m pip install --upgrade pip
)
rem

pushd %~dp0
python scripts\setup.py
popd
pause