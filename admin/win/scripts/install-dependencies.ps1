# Nextcloud Sentinel Edition - Windows Dependency Installer
#
# SPDX-FileCopyrightText: 2026 Nextcloud Sentinel Project
# SPDX-License-Identifier: GPL-2.0-or-later
#
# This script installs all required dependencies for building Nextcloud Sentinel on Windows.
# It uses KDE Craft as the primary dependency manager for KDE Frameworks.

param(
    [switch]$UseCraft,
    [switch]$UseVcpkg,
    [switch]$InstallQt,
    [switch]$InstallVS,
    [switch]$All,
    [string]$CraftRoot = "$env:USERPROFILE\CraftRoot",
    [string]$VcpkgRoot = "$env:USERPROFILE\vcpkg"
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

# ============================================================================
# Helper Functions
# ============================================================================

function Write-Header {
    param([string]$Message)
    Write-Host ""
    Write-Host "=" * 70 -ForegroundColor Cyan
    Write-Host " $Message" -ForegroundColor Cyan
    Write-Host "=" * 70 -ForegroundColor Cyan
    Write-Host ""
}

function Write-Step {
    param([string]$Message)
    Write-Host "[*] $Message" -ForegroundColor Green
}

function Write-Warning {
    param([string]$Message)
    Write-Host "[!] $Message" -ForegroundColor Yellow
}

function Write-Error {
    param([string]$Message)
    Write-Host "[X] $Message" -ForegroundColor Red
}

function Test-Admin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = New-Object Security.Principal.WindowsPrincipal($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Test-Command {
    param([string]$Command)
    $null = Get-Command $Command -ErrorAction SilentlyContinue
    return $?
}

# ============================================================================
# Install Basic Tools
# ============================================================================

function Install-BasicTools {
    Write-Header "Installing Basic Build Tools"

    # Check for winget
    if (-not (Test-Command "winget")) {
        Write-Error "winget not found. Please install App Installer from Microsoft Store."
        return $false
    }

    # Install Git
    if (-not (Test-Command "git")) {
        Write-Step "Installing Git..."
        winget install --id Git.Git -e --accept-package-agreements --accept-source-agreements
    } else {
        Write-Step "Git already installed"
    }

    # Install CMake
    if (-not (Test-Command "cmake")) {
        Write-Step "Installing CMake..."
        winget install --id Kitware.CMake -e --accept-package-agreements --accept-source-agreements
    } else {
        Write-Step "CMake already installed"
    }

    # Install Ninja
    if (-not (Test-Command "ninja")) {
        Write-Step "Installing Ninja..."
        winget install --id Ninja-build.Ninja -e --accept-package-agreements --accept-source-agreements
    } else {
        Write-Step "Ninja already installed"
    }

    # Install Python (required for Craft)
    if (-not (Test-Command "python")) {
        Write-Step "Installing Python..."
        winget install --id Python.Python.3.12 -e --accept-package-agreements --accept-source-agreements
    } else {
        Write-Step "Python already installed"
    }

    Write-Step "Basic tools installation complete"
    return $true
}

# ============================================================================
# Install Visual Studio
# ============================================================================

function Install-VisualStudio {
    Write-Header "Installing Visual Studio 2022"

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath) {
            Write-Step "Visual Studio already installed at: $vsPath"
            return $true
        }
    }

    Write-Step "Installing Visual Studio 2022 Build Tools..."
    Write-Warning "This will install Visual Studio Build Tools with C++ workload"

    # Install VS Build Tools with required components
    winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-package-agreements --accept-source-agreements `
        --override "--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --passive --wait"

    Write-Step "Visual Studio Build Tools installation complete"
    Write-Warning "You may need to restart your terminal for changes to take effect"

    return $true
}

# ============================================================================
# Install Qt
# ============================================================================

function Install-Qt {
    Write-Header "Qt Installation Instructions"

    Write-Host "Qt 6.8+ is required but cannot be installed automatically via command line."
    Write-Host ""
    Write-Host "Please follow these steps:"
    Write-Host ""
    Write-Host "1. Download Qt Online Installer from:"
    Write-Host "   https://www.qt.io/download-qt-installer" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "2. Run the installer and sign in (free account required)"
    Write-Host ""
    Write-Host "3. Select Custom Installation and choose:"
    Write-Host "   - Qt 6.8.x (or latest 6.x)"
    Write-Host "   - MSVC 2022 64-bit"
    Write-Host "   - Qt 5 Compatibility Module"
    Write-Host "   - Qt Shader Tools"
    Write-Host "   - Additional Libraries: Qt WebEngine, Qt WebChannel" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "4. Install to: C:\Qt (recommended)" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "5. After installation, set environment variable:"
    Write-Host "   `$env:Qt6_DIR = 'C:\Qt\6.8.0\msvc2022_64'" -ForegroundColor Green
    Write-Host ""

    $response = Read-Host "Press Enter to open Qt download page, or 'S' to skip"
    if ($response -ne "S" -and $response -ne "s") {
        Start-Process "https://www.qt.io/download-qt-installer"
    }
}

# ============================================================================
# Install KDE Craft
# ============================================================================

function Install-KDECraft {
    Write-Header "Installing KDE Craft"

    if (Test-Path "$CraftRoot\craft\craftenv.ps1") {
        Write-Step "KDE Craft already installed at: $CraftRoot"
        return $true
    }

    Write-Step "Setting up KDE Craft at: $CraftRoot"

    # Create Craft root directory
    if (-not (Test-Path $CraftRoot)) {
        New-Item -ItemType Directory -Path $CraftRoot | Out-Null
    }

    # Download Craft bootstrap
    $bootstrapUrl = "https://raw.githubusercontent.com/KDE/craft/master/setup/install_craft.ps1"
    $bootstrapScript = Join-Path $env:TEMP "install_craft.ps1"

    Write-Step "Downloading Craft bootstrap script..."
    Invoke-WebRequest -Uri $bootstrapUrl -OutFile $bootstrapScript

    # Run Craft bootstrap
    Write-Step "Running Craft bootstrap..."
    Push-Location $CraftRoot

    try {
        # Set up Craft with MSVC compiler
        $env:CRAFT_ROOT = $CraftRoot
        & $bootstrapScript --prefix $CraftRoot --compiler msvc2022

        if ($LASTEXITCODE -ne 0) {
            throw "Craft bootstrap failed"
        }
    }
    finally {
        Pop-Location
    }

    Write-Step "KDE Craft installation complete"

    # Install required KDE packages
    Install-KDEPackages

    return $true
}

function Install-KDEPackages {
    Write-Header "Installing KDE Frameworks via Craft"

    $craftEnv = Join-Path $CraftRoot "craft\craftenv.ps1"

    if (-not (Test-Path $craftEnv)) {
        Write-Error "Craft environment not found"
        return $false
    }

    # Source Craft environment
    Write-Step "Loading Craft environment..."
    & $craftEnv

    $packages = @(
        "kde/frameworks/extra-cmake-modules",
        "kde/frameworks/tier1/karchive",
        "kde/frameworks/tier1/ki18n",
        "kde/frameworks/tier1/kguiaddons",
        "kde/frameworks/tier1/kwidgetsaddons",
        "kde/frameworks/tier1/kwindowsystem",
        "kde/frameworks/tier1/kconfig",
        "kde/frameworks/tier2/kdoctools",
        "libs/openssl",
        "libs/sqlite",
        "libs/zlib"
    )

    foreach ($package in $packages) {
        Write-Step "Installing: $package"
        & craft $package
    }

    Write-Step "KDE Frameworks installation complete"
    return $true
}

# ============================================================================
# Install vcpkg (Alternative)
# ============================================================================

function Install-Vcpkg {
    Write-Header "Installing vcpkg"

    if (Test-Path "$VcpkgRoot\vcpkg.exe") {
        Write-Step "vcpkg already installed at: $VcpkgRoot"
    } else {
        Write-Step "Cloning vcpkg..."
        git clone https://github.com/Microsoft/vcpkg.git $VcpkgRoot

        Write-Step "Bootstrapping vcpkg..."
        Push-Location $VcpkgRoot
        try {
            & .\bootstrap-vcpkg.bat
        }
        finally {
            Pop-Location
        }
    }

    # Set environment variable
    [Environment]::SetEnvironmentVariable("VCPKG_ROOT", $VcpkgRoot, "User")
    $env:VCPKG_ROOT = $VcpkgRoot

    # Install required packages
    Write-Step "Installing vcpkg packages..."

    $packages = @(
        "openssl:x64-windows",
        "sqlite3:x64-windows",
        "zlib:x64-windows"
    )

    foreach ($package in $packages) {
        Write-Step "Installing: $package"
        & "$VcpkgRoot\vcpkg.exe" install $package
    }

    Write-Step "vcpkg installation complete"
    Write-Warning "Note: KDE Frameworks must still be installed via Craft for full build"

    return $true
}

# ============================================================================
# Main
# ============================================================================

Write-Header "Nextcloud Sentinel - Dependency Installer"

if ($All) {
    $InstallVS = $true
    $UseCraft = $true
    $InstallQt = $true
}

# Always install basic tools
Install-BasicTools

if ($InstallVS) {
    Install-VisualStudio
}

if ($InstallQt) {
    Install-Qt
}

if ($UseCraft) {
    Install-KDECraft
}

if ($UseVcpkg) {
    Install-Vcpkg
}

Write-Header "Installation Complete"

Write-Host "Next steps:"
Write-Host ""
Write-Host "1. Ensure Qt 6.8+ is installed and Qt6_DIR is set"
Write-Host "2. Open a new terminal to pick up environment changes"
Write-Host "3. Run the build script:"
Write-Host ""
Write-Host "   cd $PSScriptRoot" -ForegroundColor Green
Write-Host "   .\build-sentinel.ps1 -Configure -Build" -ForegroundColor Green
Write-Host ""

if ($UseCraft) {
    Write-Host "KDE Craft was installed. To use Craft environment:"
    Write-Host "   . $CraftRoot\craft\craftenv.ps1" -ForegroundColor Green
    Write-Host ""
}
