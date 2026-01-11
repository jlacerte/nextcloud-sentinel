# Building Nextcloud Sentinel on Windows

This guide explains how to build Nextcloud Sentinel Edition on Windows.

## Prerequisites

### Required Software

| Software | Version | Download |
|----------|---------|----------|
| Visual Studio 2022 | Build Tools or Community | [Download](https://visualstudio.microsoft.com/downloads/) |
| Qt | 6.8.0+ (MSVC 2022 64-bit) | [Download](https://www.qt.io/download-qt-installer) |
| CMake | 3.16+ | `winget install Kitware.CMake` |
| Git | Latest | `winget install Git.Git` |
| Ninja | Latest (optional) | `winget install Ninja-build.Ninja` |
| Python | 3.10+ (for KDE Craft) | `winget install Python.Python.3.12` |

### KDE Frameworks 6

Nextcloud requires KDE Frameworks 6 (KF6) which includes:
- Extra CMake Modules (ECM) 6.0+
- KArchive
- KI18n
- KGuiAddons
- KWidgetsAddons
- KConfig

These are installed via **KDE Craft**.

## Quick Start

### Automated Setup

```powershell
# 1. Install dependencies (run as Administrator for VS installation)
cd admin\win\scripts
.\install-dependencies.ps1 -All

# 2. Build the project
.\compile-sentinel.ps1 -Configure -Build
```

### Manual Setup

#### Step 1: Install Visual Studio Build Tools

```powershell
winget install Microsoft.VisualStudio.2022.BuildTools --override "--add Microsoft.VisualStudio.Workload.VCTools --add Microsoft.VisualStudio.Component.VC.Tools.x86.x64 --add Microsoft.VisualStudio.Component.Windows11SDK.22621 --passive"
```

#### Step 2: Install Qt 6.8+

1. Download Qt Online Installer: https://www.qt.io/download-qt-installer
2. Sign in (free Qt account required)
3. Select **Custom Installation**
4. Choose:
   - Qt 6.8.x (latest)
   - MSVC 2022 64-bit
   - Qt 5 Compatibility Module
   - Qt Shader Tools
   - Additional Libraries: Qt WebEngine, Qt WebChannel
5. Install to `C:\Qt`

Set environment variable:
```powershell
[Environment]::SetEnvironmentVariable("Qt6_DIR", "C:\Qt\6.8.0\msvc2022_64", "User")
```

#### Step 3: Install KDE Craft

```powershell
# Download and run Craft bootstrap
$CraftRoot = "$env:USERPROFILE\CraftRoot"
New-Item -ItemType Directory -Path $CraftRoot -Force
Set-Location $CraftRoot

Invoke-WebRequest -Uri "https://raw.githubusercontent.com/KDE/craft/master/setup/install_craft.ps1" -OutFile "install_craft.ps1"
.\install_craft.ps1 --prefix $CraftRoot --compiler msvc2022

# Install required KDE packages
. .\craft\craftenv.ps1
craft kde/frameworks/extra-cmake-modules
craft kde/frameworks/tier1/karchive
craft kde/frameworks/tier1/ki18n
craft kde/frameworks/tier1/kguiaddons
craft kde/frameworks/tier1/kconfig
craft libs/openssl
craft libs/sqlite
```

#### Step 4: Build Nextcloud Sentinel

```powershell
# Open Developer PowerShell for VS 2022, or run:
& "${env:ProgramFiles}\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Launch-VsDevShell.ps1" -Arch amd64

# Navigate to source
cd D:\nextcloud\nextcloud-sentinel

# Create build directory
mkdir build-windows
cd build-windows

# Configure
cmake .. -G Ninja `
    -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="C:\Qt\6.8.0\msvc2022_64;$env:USERPROFILE\CraftRoot" `
    -DBUILD_TESTING=ON

# Build
cmake --build . --parallel

# Run tests
ctest --output-on-failure
```

## Build Script Options

The `compile-sentinel.ps1` script supports several options:

```powershell
# Full build (configure + build)
.\compile-sentinel.ps1

# Debug build
.\compile-sentinel.ps1 -BuildType Debug

# Clean build
.\compile-sentinel.ps1 -Clean -Configure -Build

# Run tests after build
.\compile-sentinel.ps1 -Build -Test

# Specify Qt path
.\compile-sentinel.ps1 -QtPath "D:\Qt\6.8.0\msvc2022_64"

# Use specific number of parallel jobs
.\compile-sentinel.ps1 -Jobs 8
```

## Directory Structure

After building:

```
nextcloud-sentinel/
├── build-windows-Release/
│   ├── bin/
│   │   ├── nextcloud.exe          # Main application
│   │   ├── nextcloudcmd.exe       # Command-line client
│   │   └── *.dll                  # Required DLLs
│   └── ...
└── install-windows-Release/       # Install prefix (if installed)
```

## Troubleshooting

### CMake cannot find ECM

Ensure KDE Craft is installed and add the Craft root to CMAKE_PREFIX_PATH:

```powershell
cmake .. -DCMAKE_PREFIX_PATH="$env:USERPROFILE\CraftRoot;C:\Qt\6.8.0\msvc2022_64"
```

### Qt not found

Set the Qt6_DIR environment variable:

```powershell
$env:Qt6_DIR = "C:\Qt\6.8.0\msvc2022_64"
```

### LNK2019: unresolved external symbol

This usually indicates missing dependencies. Ensure all KDE Frameworks are installed via Craft:

```powershell
. $env:USERPROFILE\CraftRoot\craft\craftenv.ps1
craft --install-deps nextcloud-client
```

### Access denied during build

Run PowerShell as Administrator or check that no antivirus is blocking the build process.

### OpenSSL not found

Install OpenSSL via Craft:

```powershell
craft libs/openssl
```

Or via vcpkg:

```powershell
vcpkg install openssl:x64-windows
```

## Running Tests

### All Tests

```powershell
cd build-windows-Release
ctest --output-on-failure -C Release
```

### Kill Switch Tests Only

```powershell
ctest -R KillSwitch --output-on-failure -C Release
```

### Verbose Test Output

```powershell
ctest -V -R KillSwitch -C Release
```

## Creating an Installer

After a successful build, you can create an MSI installer:

```powershell
# Enable MSI build
cmake .. -DBUILD_WIN_MSI=ON

# Build
cmake --build . --target package
```

The installer will be in `build-windows-Release/`.

## Development Tips

### VS Code Integration

Create `.vscode/settings.json`:

```json
{
    "cmake.configureSettings": {
        "CMAKE_PREFIX_PATH": "C:/Qt/6.8.0/msvc2022_64;${env:USERPROFILE}/CraftRoot"
    },
    "cmake.generator": "Ninja"
}
```

### Debugging

For debugging, build with Debug type:

```powershell
.\compile-sentinel.ps1 -BuildType Debug
```

Then attach Visual Studio debugger to `nextcloud.exe`.

## Kill Switch Module

The Kill Switch module is located in `src/libsync/killswitch/` and includes:

- `KillSwitchManager` - Central coordinator
- `MassDeleteDetector` - Detects bulk file deletions
- `EntropyDetector` - Detects ransomware-encrypted files
- `CanaryDetector` - Monitors honeypot/canary files

Tests are in `test/testkillswitch.cpp`.
