#!/usr/bin/env pwsh
# Test standalone release package

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Testing Standalone Release" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Set directories
$TestDir = "test-standalone-release"
$SourceDir = "build-release\Release"

# Clean previous test
Write-Host "Cleaning previous test directory..." -ForegroundColor Yellow
if (Test-Path $TestDir) {
    Remove-Item $TestDir -Recurse -Force
    Write-Host "Previous test directory removed." -ForegroundColor Green
}

# Create test directory
Write-Host "`nCreating test directory..." -ForegroundColor Yellow
New-Item -ItemType Directory -Path $TestDir | Out-Null

# Copy release files
Write-Host "Copying release files..." -ForegroundColor Yellow
Copy-Item "$SourceDir\*" $TestDir -Recurse -Force
Write-Host "Files copied successfully." -ForegroundColor Green

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Verifying copied files..." -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Check critical files
$allOk = $true

# Check executable
if (Test-Path "$TestDir\CellAnalyzer.exe") {
    Write-Host "[OK] CellAnalyzer.exe found" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Missing: CellAnalyzer.exe" -ForegroundColor Red
    $allOk = $false
}

# Check OpenCV DLL
if (Test-Path "$TestDir\opencv_world4110.dll") {
    Write-Host "[OK] OpenCV DLL found" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Missing: opencv_world4110.dll" -ForegroundColor Red
    $allOk = $false
}

# Check YOLO models
$model1 = "$TestDir\ml-data\models\yolov8s_cells_v1.0.onnx"
if (Test-Path $model1) {
    $size1 = (Get-Item $model1).Length / 1MB
    Write-Host "[OK] YOLO model v1.0 found ($([math]::Round($size1, 2)) MB)" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Missing: ml-data\models\yolov8s_cells_v1.0.onnx" -ForegroundColor Red
    $allOk = $false
}

$model2 = "$TestDir\ml-data\models\yolov8s_cells_v2.0.onnx"
if (Test-Path $model2) {
    $size2 = (Get-Item $model2).Length / 1MB
    Write-Host "[OK] YOLO model v2.0 found ($([math]::Round($size2, 2)) MB)" -ForegroundColor Green
} else {
    Write-Host "[ERROR] Missing: ml-data\models\yolov8s_cells_v2.0.onnx" -ForegroundColor Red
    $allOk = $false
}

# Check settings (optional)
if (Test-Path "$TestDir\settings.json") {
    Write-Host "[OK] settings.json found" -ForegroundColor Green
} else {
    Write-Host "[WARNING] Missing: settings.json (will be created on first run)" -ForegroundColor Yellow
}

# Check RELEASE_NOTES
if (Test-Path "$TestDir\RELEASE_NOTES.md") {
    Write-Host "[OK] RELEASE_NOTES.md found" -ForegroundColor Green
} else {
    Write-Host "[WARNING] Missing: RELEASE_NOTES.md" -ForegroundColor Yellow
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Directory Structure:" -ForegroundColor Green
Write-Host "========================================" -ForegroundColor Cyan

# Show ml-data structure
if (Test-Path "$TestDir\ml-data") {
    Write-Host "`nml-data/" -ForegroundColor Cyan
    Get-ChildItem "$TestDir\ml-data" -Recurse -File | ForEach-Object {
        $relativePath = $_.FullName.Replace("$TestDir\", "")
        $sizeMB = [math]::Round($_.Length / 1MB, 2)
        Write-Host "  $relativePath ($sizeMB MB)" -ForegroundColor Gray
    }
} else {
    Write-Host "[ERROR] ml-data directory not found!" -ForegroundColor Red
    $allOk = $false
}

# Calculate total size
$totalSize = (Get-ChildItem $TestDir -Recurse | Measure-Object -Property Length -Sum).Sum / 1MB
Write-Host "`nTotal package size: $([math]::Round($totalSize, 2)) MB" -ForegroundColor Cyan

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
if ($allOk) {
    Write-Host "[SUCCESS] All critical files are present!" -ForegroundColor Green
    Write-Host ""
    Write-Host "You can now test the application by running:" -ForegroundColor Yellow
    Write-Host "  cd $TestDir" -ForegroundColor White
    Write-Host "  .\CellAnalyzer.exe" -ForegroundColor White
    Write-Host ""
    Write-Host "Or copy the entire '$TestDir' folder to another location/PC" -ForegroundColor Yellow
} else {
    Write-Host "[FAILED] Some critical files are missing!" -ForegroundColor Red
    Write-Host "Please rebuild the project with build-release.bat or build-release.ps1" -ForegroundColor Yellow
}
Write-Host "========================================" -ForegroundColor Cyan

Write-Host "`nPress any key to continue..."
$null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
