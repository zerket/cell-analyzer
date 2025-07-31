@echo off
echo Starting CellAnalyzer...
cd /d %~dp0
build\Release\CellAnalyzer.exe
pause