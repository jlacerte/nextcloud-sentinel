@echo off
:: =============================================================================
:: Nextcloud Sentinel - Double-cliquez pour démarrer!
:: =============================================================================
::
:: Ce script lance le setup PowerShell en mode administrateur.
:: Une fenêtre va s'ouvrir pour demander les droits admin - cliquez "Oui".
::
:: =============================================================================

echo.
echo  ====================================================
echo   NEXTCLOUD SENTINEL - Setup
echo  ====================================================
echo.
echo  Ce script va:
echo   1. Installer les outils necessaires
echo   2. Creer le repo GitHub
echo   3. Pousser le code
echo   4. Lancer les tests CI
echo.
echo  Une fenetre va demander les droits administrateur.
echo  Cliquez "Oui" pour continuer.
echo.
pause

:: Launch PowerShell as admin
powershell -Command "Start-Process powershell -ArgumentList '-ExecutionPolicy Bypass -File \"%~dp0SETUP-WINDOWS.ps1\"' -Verb RunAs"
