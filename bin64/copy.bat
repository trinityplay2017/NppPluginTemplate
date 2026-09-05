@echo off
:: Check if running as admin
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting administrator privileges...
    powershell -Command "Start-Process '%~f0' -Verb RunAs"
    exit /b
)

echo Closing Notepad++ gracefully...
:: Send WM_CLOSE message to all Notepad++ windows
powershell -Command "Get-Process notepad++ | ForEach-Object { $_.CloseMainWindow() }"

:: Wait for graceful shutdown
timeout /t 2 /nobreak >nul

:: Force kill if still running
taskkill /F /IM notepad++.exe 2>nul

echo Copying plugin...
copy "D:\DEV_Z\cplusplus\plugintemplate-master\bin64\JackPlugin.dll" "C:\Program Files\Notepad++\plugins\JackPlugin\"

if %errorLevel% equ 0 (
    echo Successfully copied!
) else (
    echo Failed to copy!
)

start notepad++ -multiInst -nosession
REM pause