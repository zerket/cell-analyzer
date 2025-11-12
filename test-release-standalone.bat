@echo off
echo ========================================
echo Testing Standalone Release
echo ========================================
echo.

:: Set test directory
set TEST_DIR=test-standalone-release
set SOURCE_DIR=build-release\Release

:: Clean previous test
echo Cleaning previous test directory...
if exist %TEST_DIR% (
    rmdir /S /Q %TEST_DIR%
    echo Previous test directory removed.
)

:: Create test directory
echo Creating test directory...
mkdir %TEST_DIR%

:: Copy release files
echo Copying release files...
xcopy "%SOURCE_DIR%\*" "%TEST_DIR%\" /S /E /I /Y >nul

echo.
echo ========================================
echo Verifying copied files...
echo ========================================
echo.

:: Check critical files
set MISSING=0

if not exist "%TEST_DIR%\CellAnalyzer.exe" (
    echo [ERROR] Missing: CellAnalyzer.exe
    set MISSING=1
) else (
    echo [OK] CellAnalyzer.exe found
)

if not exist "%TEST_DIR%\opencv_world4110.dll" (
    echo [ERROR] Missing: opencv_world4110.dll
    set MISSING=1
) else (
    echo [OK] OpenCV DLL found
)

if not exist "%TEST_DIR%\ml-data\models\yolov8s_cells_v1.0.onnx" (
    echo [ERROR] Missing: ml-data\models\yolov8s_cells_v1.0.onnx
    set MISSING=1
) else (
    echo [OK] YOLO model v1.0 found
)

if not exist "%TEST_DIR%\ml-data\models\yolov8s_cells_v2.0.onnx" (
    echo [ERROR] Missing: ml-data\models\yolov8s_cells_v2.0.onnx
    set MISSING=1
) else (
    echo [OK] YOLO model v2.0 found
)

if not exist "%TEST_DIR%\settings.json" (
    echo [WARNING] Missing: settings.json (will be created on first run)
) else (
    echo [OK] settings.json found
)

echo.
echo ========================================
echo Directory Structure:
echo ========================================
tree /F "%TEST_DIR%\ml-data"

echo.
echo ========================================
if %MISSING%==1 (
    echo [FAILED] Some critical files are missing!
    echo Please rebuild the project with build-release.bat
    exit /b 1
) else (
    echo [SUCCESS] All critical files are present!
    echo.
    echo You can now test the application by running:
    echo   cd %TEST_DIR%
    echo   CellAnalyzer.exe
    echo.
    echo Or copy the entire "%TEST_DIR%" folder to another location/PC
)
echo ========================================

pause
