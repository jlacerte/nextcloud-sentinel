#Requires -RunAsAdministrator
# =============================================================================
# Nextcloud Sentinel - Windows Setup Script (One-Click)
# =============================================================================
#
# CE SCRIPT FAIT TOUT AUTOMATIQUEMENT:
# 1. Installe les outils (Git, CMake, Ninja, Python, VS Build Tools)
# 2. Crée le repo GitHub et pousse le code
# 3. Lance les workflows CI
#
# USAGE: Clic-droit > "Exécuter avec PowerShell en tant qu'administrateur"
#
# =============================================================================

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# Colors
function Write-Step { param($msg) Write-Host "`n[*] $msg" -ForegroundColor Cyan }
function Write-OK { param($msg) Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warn { param($msg) Write-Host "[!] $msg" -ForegroundColor Yellow }
function Write-Err { param($msg) Write-Host "[X] $msg" -ForegroundColor Red }

Write-Host @"

    _   __          __       __                __
   / | / /__  _  __/ /______/ /___  __  ______/ /
  /  |/ / _ \| |/_/ __/ ___/ / __ \/ / / / __  /
 / /|  /  __/>  </ /_/ /__/ / /_/ / /_/ / /_/ /
/_/ |_/\___/_/|_|\__/\___/_/\____/\__,_/\__,_/

   _____ _____ _   _ _____ ___ _   _ _____ _
  /  ___|  ___| \ | |_   _|_ _| \ | | ____| |
  \ `--.| |__ |  \| | | |  | ||  \| |  _| | |
   `--. \  __|| . ` | | |  | || . ` | |___| |___
  /\__/ / |___| |\  | | | |___|_|\  |_____|_____|
  \____/\____/\_| \_/ \_/     |_| \_|

  Windows Setup Script - Version 1.0

"@ -ForegroundColor Magenta

$ProjectDir = Split-Path -Parent $MyInvocation.MyCommand.Path
Write-Host "Project: $ProjectDir" -ForegroundColor Gray
Write-Host ""

# =============================================================================
# PHASE 1: Install Tools
# =============================================================================

Write-Step "PHASE 1: Installation des outils de base"

# Check winget
if (-not (Get-Command winget -ErrorAction SilentlyContinue)) {
    Write-Err "winget non trouvé. Installez App Installer depuis le Microsoft Store."
    Read-Host "Appuyez sur Entrée pour quitter"
    exit 1
}

# Install Git
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
    Write-Step "Installation de Git..."
    winget install --id Git.Git -e --accept-package-agreements --accept-source-agreements --silent
    $env:PATH += ";C:\Program Files\Git\bin"
} else {
    Write-OK "Git déjà installé"
}

# Install GitHub CLI
if (-not (Get-Command gh -ErrorAction SilentlyContinue)) {
    Write-Step "Installation de GitHub CLI..."
    winget install --id GitHub.cli -e --accept-package-agreements --accept-source-agreements --silent
    $env:PATH += ";C:\Program Files\GitHub CLI"
} else {
    Write-OK "GitHub CLI déjà installé"
}

# Install CMake
if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    Write-Step "Installation de CMake..."
    winget install --id Kitware.CMake -e --accept-package-agreements --accept-source-agreements --silent
} else {
    Write-OK "CMake déjà installé"
}

# Install Ninja
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    Write-Step "Installation de Ninja..."
    winget install --id Ninja-build.Ninja -e --accept-package-agreements --accept-source-agreements --silent
} else {
    Write-OK "Ninja déjà installé"
}

# Install Python
if (-not (Get-Command python -ErrorAction SilentlyContinue)) {
    Write-Step "Installation de Python..."
    winget install --id Python.Python.3.12 -e --accept-package-agreements --accept-source-agreements --silent
} else {
    Write-OK "Python déjà installé"
}

Write-OK "Outils de base installés!"

# =============================================================================
# PHASE 2: GitHub Setup
# =============================================================================

Write-Step "PHASE 2: Configuration GitHub"

# Refresh PATH
$env:PATH = [System.Environment]::GetEnvironmentVariable("PATH", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("PATH", "User")

# Check gh auth
$ghStatus = gh auth status 2>&1
if ($LASTEXITCODE -ne 0) {
    Write-Warn "GitHub CLI pas authentifié. Lancement de l'authentification..."
    Write-Host ""
    Write-Host "Une fenêtre de navigateur va s'ouvrir. Connectez-vous à GitHub." -ForegroundColor Yellow
    Write-Host ""
    gh auth login --web --git-protocol https
    if ($LASTEXITCODE -ne 0) {
        Write-Err "Échec de l'authentification GitHub"
        Read-Host "Appuyez sur Entrée pour quitter"
        exit 1
    }
}
Write-OK "GitHub CLI authentifié"

# =============================================================================
# PHASE 3: Create Repo and Push
# =============================================================================

Write-Step "PHASE 3: Création du repository GitHub"

Push-Location $ProjectDir

# Check if remote already exists
$remotes = git remote -v 2>&1
if ($remotes -match "origin") {
    Write-OK "Remote 'origin' existe déjà"
    $repoUrl = git remote get-url origin
} else {
    Write-Step "Création du repo GitHub..."

    # Create repo
    $repoName = "nextcloud-sentinel"
    gh repo create $repoName --private --source=. --push 2>&1

    if ($LASTEXITCODE -eq 0) {
        Write-OK "Repository créé et code poussé!"
        $repoUrl = gh repo view --json url -q ".url"
    } else {
        Write-Warn "Le repo existe peut-être déjà. Tentative de push..."
        git push -u origin master 2>&1
    }
}

# Get repo URL
$repoUrl = gh repo view --json url -q ".url" 2>$null
if (-not $repoUrl) {
    $repoUrl = git remote get-url origin 2>$null
}

Pop-Location

Write-OK "Repository: $repoUrl"

# =============================================================================
# PHASE 4: Check CI
# =============================================================================

Write-Step "PHASE 4: Vérification des workflows CI"

Write-Host ""
Write-Host "Les workflows GitHub Actions devraient démarrer automatiquement." -ForegroundColor Yellow
Write-Host ""

if ($repoUrl) {
    $actionsUrl = "$repoUrl/actions"
    Write-Host "URL des Actions: $actionsUrl" -ForegroundColor Cyan
    Write-Host ""

    $response = Read-Host "Voulez-vous ouvrir GitHub Actions dans le navigateur? (O/n)"
    if ($response -ne "n" -and $response -ne "N") {
        Start-Process $actionsUrl
    }
}

# =============================================================================
# SUMMARY
# =============================================================================

Write-Host ""
Write-Host "=" * 60 -ForegroundColor Green
Write-Host " SETUP TERMINÉ!" -ForegroundColor Green
Write-Host "=" * 60 -ForegroundColor Green
Write-Host ""
Write-Host "Repository: $repoUrl" -ForegroundColor Cyan
Write-Host "Actions:    $repoUrl/actions" -ForegroundColor Cyan
Write-Host ""
Write-Host "Prochaines étapes:" -ForegroundColor Yellow
Write-Host "1. Vérifier les workflows CI sur GitHub Actions"
Write-Host "2. Installer Qt 6.8 (manuel - voir SENTINEL-BUILD-PLAN.md)"
Write-Host "3. Exécuter: .\admin\win\scripts\install-dependencies.ps1 -UseCraft"
Write-Host "4. Exécuter: .\admin\win\scripts\compile-sentinel.ps1"
Write-Host ""

Read-Host "Appuyez sur Entrée pour terminer"
