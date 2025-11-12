@echo off
echo ========================================
echo Building CellAnalyzer Release Version
echo ========================================

:: Set paths
set BUILD_DIR=build-release
set OPENCV_BIN=D:\opencv\build\x64\vc16\bin

:: Clean previous build
echo Cleaning previous build...
if exist %BUILD_DIR% (
    rmdir /S /Q %BUILD_DIR%
    echo Previous build directory removed.
)

:: Create build directory
echo Creating build directory...
mkdir %BUILD_DIR%
cd %BUILD_DIR%

:: Configure with CMake
echo Configuring project with CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    exit /b 1
)

:: Build the project
echo Building project...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

:: Deploy Qt dependencies
echo Deploying Qt dependencies...
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw Release\CellAnalyzer.exe
if %ERRORLEVEL% NEQ 0 (
    echo Qt deployment failed!
    exit /b 1
)

:: Copy OpenCV DLLs
echo Copying OpenCV libraries...
copy "%OPENCV_BIN%\opencv_world4110.dll" "Release\" >nul
copy "%OPENCV_BIN%\opencv_videoio_msmf4110_64.dll" "Release\" >nul
copy "%OPENCV_BIN%\opencv_videoio_ffmpeg4110_64.dll" "Release\" >nul

:: Copy documentation and settings
echo Copying documentation and settings files...
copy "..\CHANGELOG.md" "Release\" >nul
copy "..\CLAUDE.md" "Release\" >nul
copy "..\README.md" "Release\" >nul
copy "..\RELEASE_NOTES.md" "Release\" >nul
copy "..\todo.md" "Release\" >nul
copy "..\settings.json" "Release\" >nul

:: Copy ML data (models and scripts)
echo Copying ML data (models and scripts)...
if not exist "Release\ml-data\models" mkdir "Release\ml-data\models"
if not exist "Release\ml-data\scripts" mkdir "Release\ml-data\scripts"
if not exist "Release\ml-data\docs" mkdir "Release\ml-data\docs"

:: Copy ONNX models (required for inference)
copy "..\ml-data\models\*.onnx" "Release\ml-data\models\" >nul
echo   - ONNX models copied

:: Copy Python scripts (if needed for fallback)
copy "..\ml-data\scripts\*.py" "Release\ml-data\scripts\" >nul
echo   - Python scripts copied

:: Copy ML documentation
copy "..\ml-data\*.md" "Release\ml-data\" >nul
copy "..\ml-data\docs\*.md" "Release\ml-data\docs\" >nul
echo   - ML documentation copied

:: Return to original directory
cd ..

:: Show build info
echo.
echo ========================================
echo Build completed successfully!
echo ========================================
echo Release location: %BUILD_DIR%\Release\
echo Executable: %BUILD_DIR%\Release\CellAnalyzer.exe
echo.

:: Calculate and show build size
for /f "tokens=3" %%a in ('dir %BUILD_DIR%\Release\ /s ^| find "File(s)"') do set size=%%a
echo Total build size: %size% bytes
echo ========================================

pause