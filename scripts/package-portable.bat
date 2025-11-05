@echo off
echo Creating portable package for CellAnalyzer...
echo.

:: Check if portable build exists
if not exist "portable\CellAnalyzer\CellAnalyzer.exe" (
    echo ERROR: Portable build not found. Run build-portable.bat first.
    exit /b 1
)

:: Get current date for filename
for /f "tokens=2 delims==" %%I in ('wmic os get localdatetime /value') do set datetime=%%I
set VERSION=%datetime:~0,4%-%datetime:~4,2%-%datetime:~6,2%

:: Create package filename
set PACKAGE_NAME=CellAnalyzer-Portable-Win64-%VERSION%.zip

:: Check for 7-Zip or use PowerShell
where 7z >nul 2>nul
if %errorlevel%==0 (
    echo Using 7-Zip to create package...
    7z a -tzip "%PACKAGE_NAME%" ".\portable\CellAnalyzer\*" -r
) else (
    echo Using PowerShell to create package...
    powershell -Command "Compress-Archive -Path 'portable\CellAnalyzer' -DestinationPath '%PACKAGE_NAME%' -Force"
)

if exist "%PACKAGE_NAME%" (
    echo.
    echo Package created successfully: %PACKAGE_NAME%
    echo.
    :: Calculate size
    for %%F in ("%PACKAGE_NAME%") do set SIZE=%%~zF
    set /a SIZE_MB=%SIZE% / 1048576
    echo Package size: %SIZE_MB% MB
    echo.
    echo This package includes:
    echo - CellAnalyzer.exe (main executable)
    echo - All required Qt 6 libraries
    echo - OpenCV libraries
    echo - Image format plugins
    echo - Platform plugins
    echo - README.txt with instructions
    echo.
    echo The package should work on:
    echo - Windows 7 SP1 (64-bit) with Visual C++ 2022 Redistributable
    echo - Windows 10 (64-bit)
    echo - Windows 11 (64-bit)
) else (
    echo ERROR: Failed to create package!
    exit /b 1
)