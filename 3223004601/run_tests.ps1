<#
Run example tests for the C++ program on Windows

This script looks for an executable `main.exe` in the repository root and runs it
with example files in the `tests` directory, then compares the produced output
with expected output files (*.out). Adjust paths if your build outputs elsewhere.
#>

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe = Join-Path $root "..\main.exe"

if (-Not (Test-Path $exe)) {
    Write-Error "Executable not found: $exe. Build the project to produce main.exe first."
    exit 2
}

$tests = Get-ChildItem -Path $root -Filter "*.in"
$failures = @()

foreach ($t in $tests) {
    $base = [System.IO.Path]::GetFileNameWithoutExtension($t.Name)
    $outFile = Join-Path $root ("$base.out")
    $ansFile = Join-Path $root ("$base.ans")

    Write-Host "Running test: $($t.Name)"
    & $exe $t.FullName $t.FullName $outFile

    if (-Not (Test-Path $outFile)) {
        Write-Warning "Output file not produced: $outFile"
        $failures += $t.Name
        continue
    }

    if (Test-Path $ansFile) {
        $expected = Get-Content $ansFile -Raw
        $actual = Get-Content $outFile -Raw
        if ($expected.Trim() -ne $actual.Trim()) {
            Write-Host "  FAILED: output differs"
            Write-Host "  Expected:`n$expected" -ForegroundColor Yellow
            Write-Host "  Actual:`n$actual" -ForegroundColor Red
            $failures += $t.Name
        }
        else {
            Write-Host "  OK" -ForegroundColor Green
        }
    }
    else {
        Write-Host "  No .ans file to compare against. Produced: $outFile"
    }
}

if ($failures.Count -gt 0) {
    Write-Host "Tests failed: $($failures -join ', ')" -ForegroundColor Red
    exit 1
}
else {
    Write-Host "All tests passed" -ForegroundColor Green
    exit 0
}
