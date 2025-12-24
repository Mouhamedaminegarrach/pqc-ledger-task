# Build CLI Executable Script
# This script builds the pqc-ledger-cli executable

Write-Host "=== Building pqc-ledger-cli ===" -ForegroundColor Cyan

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "Error: build directory not found!" -ForegroundColor Red
    Write-Host "Please run: mkdir build; cd build; cmake .. -DCMAKE_BUILD_TYPE=Release" -ForegroundColor Yellow
    exit 1
}

# Try to find cmake
$cmake = $null
$cmakePaths = @(
    "cmake",
    "C:\Program Files\CMake\bin\cmake.exe",
    "C:\Program Files (x86)\CMake\bin\cmake.exe",
    "$env:LOCALAPPDATA\Programs\CMake\bin\cmake.exe",
    "$env:USERPROFILE\Downloads\cmake-*\bin\cmake.exe"
)

foreach ($path in $cmakePaths) {
    if ($path -like "*\*") {
        # Handle wildcard paths
        $found = Get-ChildItem -Path (Split-Path $path) -Filter (Split-Path -Leaf $path) -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            $cmake = $found.FullName
            break
        }
    } else {
        $cmd = Get-Command $path -ErrorAction SilentlyContinue
        if ($cmd) {
            $cmake = $cmd.Source
            break
        }
    }
}

if (-not $cmake) {
    Write-Host "Error: CMake not found!" -ForegroundColor Red
    Write-Host ""
    Write-Host "Please install CMake or add it to your PATH." -ForegroundColor Yellow
    Write-Host "Download from: https://cmake.org/download/" -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Alternatively, you can:" -ForegroundColor Yellow
    Write-Host "1. Open build\pqc-ledger-task.sln in Visual Studio" -ForegroundColor Yellow
    Write-Host "2. Right-click 'pqc-ledger-cli' project -> Build" -ForegroundColor Yellow
    exit 1
}

Write-Host "Found CMake: $cmake" -ForegroundColor Green

# Build the CLI
Write-Host ""
Write-Host "Building pqc-ledger-cli..." -ForegroundColor Cyan
& $cmake --build build --config Release --target pqc-ledger-cli

if ($LASTEXITCODE -eq 0) {
    Write-Host ""
    Write-Host "=== Build Successful! ===" -ForegroundColor Green
    
    # Check if executable exists
    $cliPath = "build\bin\Release\pqc-ledger-cli.exe"
    if (Test-Path $cliPath) {
        Write-Host "CLI executable: $cliPath" -ForegroundColor Green
        $fileInfo = Get-Item $cliPath
        Write-Host "Size: $($fileInfo.Length) bytes" -ForegroundColor Gray
        Write-Host "Date: $($fileInfo.LastWriteTime)" -ForegroundColor Gray
    } else {
        Write-Host "Warning: Executable not found at expected location: $cliPath" -ForegroundColor Yellow
        Write-Host "Searching for executable..." -ForegroundColor Yellow
        $found = Get-ChildItem -Path build -Recurse -Filter "pqc-ledger-cli.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($found) {
            Write-Host "Found at: $($found.FullName)" -ForegroundColor Green
        }
    }
} else {
    Write-Host ""
    Write-Host "=== Build Failed! ===" -ForegroundColor Red
    Write-Host "Exit code: $LASTEXITCODE" -ForegroundColor Red
    exit 1
}

