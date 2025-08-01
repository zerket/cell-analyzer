@echo off
echo Preparing CellAnalyzer for Windows 7 compatibility...
echo.

:: Download Visual C++ Redistributable installers
echo Windows 7 users need to install Visual C++ Redistributables.
echo.
echo Please download and include these redistributables:
echo.
echo 1. Visual C++ 2015-2022 Redistributable (x64):
echo    https://aka.ms/vs/17/release/vc_redist.x64.exe
echo.
echo 2. Universal C Runtime for Windows 7:
echo    https://support.microsoft.com/en-us/topic/update-for-universal-c-runtime-in-windows-c0514201-7fe6-95a3-b0a5-287930f3560c
echo.

:: Create install script for end users
echo Creating Windows 7 setup script...
echo @echo off > portable\CellAnalyzer\setup-win7.bat
echo echo CellAnalyzer Setup for Windows 7 >> portable\CellAnalyzer\setup-win7.bat
echo echo ================================ >> portable\CellAnalyzer\setup-win7.bat
echo echo. >> portable\CellAnalyzer\setup-win7.bat
echo echo This script will check for required components. >> portable\CellAnalyzer\setup-win7.bat
echo echo. >> portable\CellAnalyzer\setup-win7.bat
echo echo Checking for Visual C++ Runtime... >> portable\CellAnalyzer\setup-win7.bat
echo if not exist "%%SystemRoot%%\System32\msvcp140.dll" ( >> portable\CellAnalyzer\setup-win7.bat
echo     echo. >> portable\CellAnalyzer\setup-win7.bat
echo     echo Visual C++ 2022 Runtime not found! >> portable\CellAnalyzer\setup-win7.bat
echo     echo Please install vc_redist.x64.exe first. >> portable\CellAnalyzer\setup-win7.bat
echo     echo. >> portable\CellAnalyzer\setup-win7.bat
echo     echo Download from: >> portable\CellAnalyzer\setup-win7.bat
echo     echo https://aka.ms/vs/17/release/vc_redist.x64.exe >> portable\CellAnalyzer\setup-win7.bat
echo     echo. >> portable\CellAnalyzer\setup-win7.bat
echo     pause >> portable\CellAnalyzer\setup-win7.bat
echo     exit /b 1 >> portable\CellAnalyzer\setup-win7.bat
echo ) >> portable\CellAnalyzer\setup-win7.bat
echo echo Visual C++ Runtime: OK >> portable\CellAnalyzer\setup-win7.bat
echo echo. >> portable\CellAnalyzer\setup-win7.bat
echo echo Setup complete! You can now run CellAnalyzer.exe >> portable\CellAnalyzer\setup-win7.bat
echo pause >> portable\CellAnalyzer\setup-win7.bat

:: Create minimal Qt configuration for better compatibility
echo Creating Qt configuration...
echo [Paths] > portable\CellAnalyzer\qt.conf
echo Plugins = . >> portable\CellAnalyzer\qt.conf

echo.
echo Windows 7 preparation complete!
echo.
echo For Windows 7 users:
echo 1. They must have Windows 7 SP1 with latest updates
echo 2. They must install Visual C++ 2015-2022 Redistributable (x64)
echo 3. They should run setup-win7.bat to check compatibility
echo.