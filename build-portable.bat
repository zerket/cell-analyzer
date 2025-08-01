@echo off
echo Building portable CellAnalyzer for Windows 7/10/11 x64...
echo.

:: Clean previous build
if exist "portable" rmdir /s /q "portable"
if exist "build-portable" rmdir /s /q "build-portable"

:: Create build directory
mkdir build-portable
cd build-portable

:: Configure with Release mode
echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
if errorlevel 1 goto :error

:: Build the project
echo Building project...
cmake --build . --config Release
if errorlevel 1 goto :error

:: Create portable directory structure
cd ..
mkdir portable
mkdir portable\CellAnalyzer

:: Copy executable
echo Copying executable...
copy build-portable\Release\CellAnalyzer.exe portable\CellAnalyzer\

:: Deploy Qt dependencies
echo Deploying Qt dependencies...
cd portable\CellAnalyzer
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw --no-virtualkeyboard --no-angle --no-webkit2 --no-quick-import --no-debug --no-opengl CellAnalyzer.exe
cd ..\..

:: Copy OpenCV DLLs
echo Copying OpenCV DLLs...
copy "D:\opencv\build\x64\vc16\bin\opencv_world4110.dll" "portable\CellAnalyzer\"
copy "D:\opencv\build\x64\vc16\bin\opencv_videoio_msmf4110_64.dll" "portable\CellAnalyzer\"
copy "D:\opencv\build\x64\vc16\bin\opencv_videoio_ffmpeg4110_64.dll" "portable\CellAnalyzer\"

:: Copy Visual C++ Redistributables (for Windows 7 compatibility)
echo Copying Visual C++ Redistributables...
:: These are usually in: C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\
:: You may need to adjust the path based on your Visual Studio installation
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC143.CRT" (
    copy "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC143.CRT\*.dll" "portable\CellAnalyzer\"
) else if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC143.CRT" (
    copy "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\VC\Redist\MSVC\14.40.33807\x64\Microsoft.VC143.CRT\*.dll" "portable\CellAnalyzer\"
) else (
    echo WARNING: Could not find Visual C++ Redistributables. 
    echo Please copy the following DLLs manually:
    echo - msvcp140.dll
    echo - vcruntime140.dll
    echo - vcruntime140_1.dll
    echo - concrt140.dll
)

:: Create README
echo Creating README...
echo CellAnalyzer Portable Version > portable\README.txt
echo ============================ >> portable\README.txt
echo. >> portable\README.txt
echo This is a portable version of CellAnalyzer that works on Windows 7, 10, and 11 (64-bit). >> portable\README.txt
echo. >> portable\README.txt
echo Requirements: >> portable\README.txt
echo - Windows 7 SP1, Windows 10, or Windows 11 (64-bit) >> portable\README.txt
echo - Visual C++ Redistributable 2022 (included) >> portable\README.txt
echo. >> portable\README.txt
echo To run: >> portable\README.txt
echo 1. Extract the entire CellAnalyzer folder to any location >> portable\README.txt
echo 2. Double-click CellAnalyzer.exe >> portable\README.txt
echo. >> portable\README.txt
echo Note: Settings will be saved in the user's AppData folder. >> portable\README.txt

:: Create a batch file to run with proper environment
echo Creating launcher...
echo @echo off > portable\CellAnalyzer\run.bat
echo cd /d "%%~dp0" >> portable\CellAnalyzer\run.bat
echo start "" CellAnalyzer.exe >> portable\CellAnalyzer\run.bat

echo.
echo Build complete! Portable version is in the 'portable' directory.
echo.
echo To make it fully portable, you may want to:
echo 1. Test on a clean Windows 7 machine
echo 2. Include Visual C++ Redistributable installer if needed
echo 3. Create a ZIP archive of the portable\CellAnalyzer folder
goto :end

:error
echo.
echo ERROR: Build failed!
exit /b 1

:end
cd ..