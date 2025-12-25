# Benchmark Runner Script (like cargo bench)
# Builds and runs the benchmark, then generates graph

param(
    [switch]$NoGraph = $false
)

Write-Host "=== PQC Ledger Benchmark Runner ===" -ForegroundColor Cyan
Write-Host ""

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

# Build the benchmark
Write-Host "Building benchmark..." -ForegroundColor Yellow
Set-Location build
& cmake --build . --config Release --target pqc-ledger-bench
if ($LASTEXITCODE -ne 0) {
    Write-Host "Build failed" -ForegroundColor Red
    Set-Location ..
    exit 1
}
Set-Location ..

# Find the benchmark executable
$benchExe = $null
$possiblePaths = @(
    "build\bin\Release\pqc-ledger-bench.exe",
    "build\Release\pqc-ledger-bench.exe"
)

foreach ($path in $possiblePaths) {
    if (Test-Path $path) {
        $benchExe = $path
        break
    }
}

if (-not $benchExe) {
    Write-Host "Error: pqc-ledger-bench.exe not found" -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=== Running Benchmark ===" -ForegroundColor Cyan
Write-Host "Benchmark: Verify 100 PQ-Signed Transactions" -ForegroundColor Gray
Write-Host ""

# Run benchmark with CSV output
$csvFile = "build\benchmark_results.csv"
& $benchExe --benchmark_format=csv | Tee-Object -FilePath $csvFile

$benchResult = $LASTEXITCODE

if ($benchResult -ne 0) {
    Write-Host "`nBenchmark failed (exit code: $benchResult)" -ForegroundColor Red
    exit $benchResult
}

Write-Host ""
Write-Host "=== Benchmark Results ===" -ForegroundColor Green
Write-Host "CSV output saved to: $csvFile" -ForegroundColor Gray

# Generate graph if requested
if (-not $NoGraph) {
    Write-Host ""
    Write-Host "=== Generating Graph ===" -ForegroundColor Cyan
    
    $python = Get-Command python -ErrorAction SilentlyContinue
    if (-not $python) {
        $python = Get-Command python3 -ErrorAction SilentlyContinue
    }
    
    if ($python) {
        & $python scripts/generate_benchmark_graph.py $csvFile
        if ($LASTEXITCODE -eq 0) {
            Write-Host ""
            Write-Host "Graph saved to: build\benchmark_graph.png" -ForegroundColor Green
        } else {
            Write-Host "Graph generation failed (but CSV is available)" -ForegroundColor Yellow
        }
    } else {
        Write-Host "Python not found. Skipping graph generation." -ForegroundColor Yellow
        Write-Host "To generate graph manually:" -ForegroundColor Gray
        Write-Host "  python scripts/generate_benchmark_graph.py $csvFile" -ForegroundColor Gray
    }
}

Write-Host ""
Write-Host "=== Benchmark Complete ===" -ForegroundColor Green
Write-Host ""
Write-Host "Results:" -ForegroundColor Cyan
Write-Host "  CSV: $csvFile" -ForegroundColor Gray
if (-not $NoGraph) {
    Write-Host "  Graph: build\benchmark_graph.png" -ForegroundColor Gray
}

exit 0

