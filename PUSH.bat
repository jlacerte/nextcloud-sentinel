@echo off
cd /d "%~dp0"
echo Pushing to GitHub...
git push origin master
echo.
echo Done!
pause
