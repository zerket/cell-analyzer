#!/usr/bin/env pwsh
# CellAnalyzer Release Build Script

Write-Host "========================================"
Write-Host "Building CellAnalyzer Release Version" -ForegroundColor Green
Write-Host "========================================"

# Configuration
$BuildDir = "build-release"
$OpenCVBin = "D:\opencv\build\x64\vc16\bin"
$Generator = "Visual Studio 17 2022"
$Architecture = "x64"

# Clean previous build
Write-Host "`nCleaning previous build..." -ForegroundColor Yellow
if (Test-Path $BuildDir) {
    Remove-Item $BuildDir -Recurse -Force
    Write-Host "Previous build directory removed." -ForegroundColor Green
}

# Create build directory
Write-Host "`nCreating build directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $BuildDir | Out-Null
Set-Location $BuildDir

try {
    # Configure with CMake
    Write-Host "`nConfiguring project with CMake..." -ForegroundColor Yellow
    $configResult = cmake -G $Generator -A $Architecture -DCMAKE_BUILD_TYPE=Release ..
    if ($LASTEXITCODE -ne 0) {
        throw "CMake configuration failed!"
    }

    # Build the project
    Write-Host "`nBuilding project..." -ForegroundColor Yellow
    $buildResult = cmake --build . --config Release
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed!"
    }

    # Deploy Qt dependencies
    Write-Host "`nDeploying Qt dependencies..." -ForegroundColor Yellow
    $deployResult = windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw Release\CellAnalyzer.exe
    if ($LASTEXITCODE -ne 0) {
        throw "Qt deployment failed!"
    }

    # Copy OpenCV DLLs
    Write-Host "`nCopying OpenCV libraries..." -ForegroundColor Yellow
    $opencvDlls = @(
        "opencv_world4110.dll",
        "opencv_videoio_msmf4110_64.dll",
        "opencv_videoio_ffmpeg4110_64.dll"
    )
    
    foreach ($dll in $opencvDlls) {
        $sourcePath = Join-Path $OpenCVBin $dll
        $destPath = Join-Path "Release" $dll
        if (Test-Path $sourcePath) {
            Copy-Item $sourcePath $destPath -Force
            Write-Host "  - Copied $dll" -ForegroundColor Gray
        } else {
            Write-Host "  - Warning: $dll not found!" -ForegroundColor Red
        }
    }

    # Return to original directory
    Set-Location ..

    # Show build info
    Write-Host "`n========================================"
    Write-Host "Build completed successfully!" -ForegroundColor Green
    Write-Host "========================================"
    Write-Host "Release location: $BuildDir\Release\"
    Write-Host "Executable: $BuildDir\Release\CellAnalyzer.exe"
    
    # Calculate and show build size
    $buildSize = (Get-ChildItem "$BuildDir\Release" -Recurse | Measure-Object -Property Length -Sum).Sum
    $buildSizeMB = [math]::Round($buildSize / 1MB, 2)
    Write-Host "`nTotal build size: $buildSizeMB MB"
    Write-Host "========================================"

    # Optional: Create ZIP archive
    $createZip = Read-Host "`nCreate ZIP archive? (y/n)"
    if ($createZip -eq 'y') {
        $zipName = "CellAnalyzer-Release-$(Get-Date -Format 'yyyy-MM-dd').zip"
        Write-Host "Creating $zipName..." -ForegroundColor Yellow
        Compress-Archive -Path "$BuildDir\Release\*" -DestinationPath $zipName -Force
        Write-Host "Archive created: $zipName" -ForegroundColor Green
    }

} catch {
    Write-Host "`nError: $_" -ForegroundColor Red
    Set-Location ..
    exit 1
}

Write-Host "`nPress any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")