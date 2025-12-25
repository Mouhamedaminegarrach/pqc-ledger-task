# Build and Run Validation Tests Script
# This script builds the validation tests and runs them, then generates a results document

Write-Host "=== Building Validation Tests ===" -ForegroundColor Cyan

# Check if we're in the right directory
if (-not (Test-Path "CMakeLists.txt")) {
    Write-Host "Error: Must run from project root directory" -ForegroundColor Red
    exit 1
}

# Check if build directory exists
if (-not (Test-Path "build")) {
    Write-Host "Creating build directory..." -ForegroundColor Yellow
    New-Item -ItemType Directory -Path "build" | Out-Null
}

# Try to find CMake
$cmake = Get-Command cmake -ErrorAction SilentlyContinue
if (-not $cmake) {
    Write-Host "Error: CMake not found in PATH" -ForegroundColor Red
    Write-Host "Please install CMake or add it to your PATH" -ForegroundColor Yellow
    exit 1
}

# Configure if needed
if (-not (Test-Path "build\CMakeCache.txt")) {
    Write-Host "Configuring CMake..." -ForegroundColor Yellow
    Set-Location build
    & cmake .. -DCMAKE_BUILD_TYPE=Release
    if ($LASTEXITCODE -ne 0) {
        Write-Host "CMake configuration failed" -ForegroundColor Red
        Set-Location ..
        exit 1
    }
    Set-Location ..
}

# Build the test
Write-Host "Building test_validation_tests..." -ForegroundColor Yellow
Set-Location build
& cmake --build . --config Release --target test_validation_tests
$buildSuccess = $LASTEXITCODE -eq 0
Set-Location ..

if (-not $buildSuccess) {
    Write-Host "Build failed. Trying to build all tests..." -ForegroundColor Yellow
    Set-Location build
    & cmake --build . --config Release --target tests
    Set-Location ..
}

# Find the test executable
$testExe = $null
$possiblePaths = @(
    "build\bin\Release\test_validation_tests.exe",
    "build\tests\Release\test_validation_tests.exe",
    "build\Release\test_validation_tests.exe"
)

foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $testExe = $path
        break
    }
}

if (-not $testExe) {
    Write-Host "Error: test_validation_tests.exe not found" -ForegroundColor Red
    Write-Host "Searched in:" -ForegroundColor Yellow
    foreach ($path in $possiblePaths) {
        Write-Host "  - $path" -ForegroundColor Gray
    }
    exit 1
}

Write-Host "`n=== Running Validation Tests ===" -ForegroundColor Cyan
Write-Host "Test executable: $testExe" -ForegroundColor Gray

# Run the tests and capture output
$outputFile = "validation_tests_output.txt"
$xmlFile = "validation_tests_results.xml"

& $testExe --gtest_output=xml:$xmlFile 2>&1 | Tee-Object -FilePath $outputFile

$testResult = $LASTEXITCODE

Write-Host "`n=== Test Results ===" -ForegroundColor Cyan
if ($testResult -eq 0) {
    Write-Host "All tests PASSED!" -ForegroundColor Green
} else {
    Write-Host "Some tests FAILED (exit code: $testResult)" -ForegroundColor Red
}

Write-Host "`nOutput saved to: $outputFile" -ForegroundColor Gray
Write-Host "XML results saved to: $xmlFile" -ForegroundColor Gray

# Generate documentation
Write-Host "`n=== Generating Documentation ===" -ForegroundColor Cyan
& pwsh -File "document_test_results.ps1" -OutputFile $outputFile -XmlFile $xmlFile

exit $testResult

