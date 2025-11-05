@echo off
echo Checking dependencies for CellAnalyzer portable build...
echo.

set MISSING_DEPS=0

:: Check for required DLLs in the portable folder
cd portable\CellAnalyzer 2>nul
if errorlevel 1 (
    echo ERROR: Portable build directory not found. Run build-portable.bat first.
    exit /b 1
)

echo Checking Qt dependencies...
for %%F in (Qt6Core.dll Qt6Gui.dll Qt6Widgets.dll Qt6Network.dll Qt6Svg.dll) do (
    if not exist "%%F" (
        echo   MISSING: %%F
        set MISSING_DEPS=1
    ) else (
        echo   OK: %%F
    )
)

echo.
echo Checking OpenCV dependencies...
for %%F in (opencv_world4110.dll opencv_videoio_msmf4110_64.dll opencv_videoio_ffmpeg4110_64.dll) do (
    if not exist "%%F" (
        echo   MISSING: %%F
        set MISSING_DEPS=1
    ) else (
        echo   OK: %%F
    )
)

echo.
echo Checking Visual C++ Runtime dependencies...
for %%F in (msvcp140.dll vcruntime140.dll vcruntime140_1.dll) do (
    if not exist "%%F" (
        echo   WARNING: %%F not found - may be required on Windows 7
    ) else (
        echo   OK: %%F
    )
)

echo.
echo Checking platform plugins...
if not exist "platforms\qwindows.dll" (
    echo   MISSING: platforms\qwindows.dll
    set MISSING_DEPS=1
) else (
    echo   OK: platforms\qwindows.dll
)

echo.
echo Checking image format plugins...
for %%F in (qico.dll qjpeg.dll) do (
    if not exist "imageformats\%%F" (
        echo   WARNING: imageformats\%%F not found
    ) else (
        echo   OK: imageformats\%%F
    )
)

echo.
if %MISSING_DEPS%==1 (
    echo RESULT: Some required dependencies are missing!
    echo Please run windeployqt or copy missing DLLs manually.
) else (
    echo RESULT: All required dependencies found.
    echo The application should run on Windows 7/10/11 x64.
)

cd ..\..