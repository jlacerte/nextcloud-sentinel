@echo off
cd /d "%~dp0"
echo Triggering Sentinel CI workflows...
echo.

echo [1/2] Linux Sentinel CI...
gh workflow run linux-sentinel-ci.yml --ref master
if %ERRORLEVEL% NEQ 0 (
    echo FAILED - Make sure gh CLI is installed and authenticated
    echo Run: gh auth login
    pause
    exit /b 1
)

echo [2/2] Windows Sentinel CI...
gh workflow run windows-sentinel-ci.yml --ref master
if %ERRORLEVEL% NEQ 0 (
    echo FAILED
    pause
    exit /b 1
)

echo.
echo CI workflows triggered successfully!
echo Check status at: https://github.com/jlacerte/nextcloud-sentinel/actions
echo.
pause
