# CellAnalyzer - Build Instructions

## Prerequisites

### Required Software

1. **Visual Studio 2022** (Community/Professional/Enterprise)
   - Workload: "Desktop development with C++"
   - Component: "MSVC v143 - VS 2022 C++ x64/x86 build tools"

2. **CMake 3.16+**
   - Download from: https://cmake.org/download/
   - Add to PATH during installation

3. **Qt 6.x**
   - Download Qt Online Installer: https://www.qt.io/download-qt-installer
   - Required components:
     - Qt 6.x for MSVC 2022 64-bit
     - Qt Creator (optional)

4. **OpenCV 4.11.0**
   - Pre-built package for Windows: https://opencv.org/releases/
   - Extract to `D:\opencv\` (or update paths in build scripts)

5. **Python 3.8+** (optional, for model training/testing)
   - Download from: https://www.python.org/downloads/
   - Packages: `pip install ultralytics opencv-python numpy torch`

### Environment Setup

1. Add Qt to PATH:
   ```cmd
   set PATH=C:\Qt\6.x.x\msvc2022_64\bin;%PATH%
   ```

2. Add CMake to PATH (if not done during installation):
   ```cmd
   set PATH=C:\Program Files\CMake\bin;%PATH%
   ```

3. Verify installations:
   ```cmd
   cmake --version
   qmake --version
   cl  # MSVC compiler (should be available in VS Developer Command Prompt)
   ```

## Build Methods

### Method 1: Batch Script (Recommended for Windows)

```cmd
# Build release version
build-release.bat

# Test standalone package
test-release-standalone.bat
```

**What it does:**
1. Cleans previous build
2. Configures CMake with VS 2022
3. Builds Release configuration
4. Deploys Qt dependencies with windeployqt
5. Copies OpenCV DLLs
6. **Copies ml-data folder (ONNX models, scripts)**
7. Copies documentation and settings

### Method 2: PowerShell Script

```powershell
# Build release version
.\build-release.ps1

# Test standalone package
.\test-release-standalone.ps1
```

**Additional features:**
- Colored output
- Better error handling
- Optional ZIP archive creation
- Detailed file size reporting

### Method 3: Manual CMake Build

```cmd
# Create build directory
mkdir build-release
cd build-release

# Configure
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build . --config Release

# Deploy Qt
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw Release\CellAnalyzer.exe

# Copy OpenCV DLLs
copy "D:\opencv\build\x64\vc16\bin\opencv_world4110.dll" "Release\"
copy "D:\opencv\build\x64\vc16\bin\opencv_videoio_msmf4110_64.dll" "Release\"
copy "D:\opencv\build\x64\vc16\bin\opencv_videoio_ffmpeg4110_64.dll" "Release\"

# Copy ML data (CRITICAL!)
xcopy "..\ml-data\models\*.onnx" "Release\ml-data\models\" /S /Y
xcopy "..\ml-data\scripts\*.py" "Release\ml-data\scripts\" /S /Y
xcopy "..\ml-data\*.md" "Release\ml-data\" /Y
```

## Output Structure

After successful build, `build-release\Release\` will contain:

```
Release/
├── CellAnalyzer.exe           # Main executable
├── *.dll                      # Qt and OpenCV libraries
├── platforms/                 # Qt plugins
├── styles/                    # Qt styles
├── imageformats/              # Qt image codecs
├── ml-data/                   # ML models and scripts
│   ├── models/
│   │   ├── yolov8s_cells_v1.0.onnx  # ~25 MB
│   │   └── yolov8s_cells_v2.0.onnx  # ~25 MB
│   ├── scripts/
│   │   ├── detect_cells.py
│   │   ├── test_model.py
│   │   └── ...
│   └── docs/
│       ├── MODEL_OUTPUT.md
│       └── TRAINING.md
├── settings.json              # Application settings
├── README.md                  # Project documentation
├── RELEASE_NOTES.md           # Release documentation
└── CHANGELOG.md               # Version history
```

## Verification

### Quick Test

```cmd
# Run from build directory
cd build-release\Release
CellAnalyzer.exe
```

### Standalone Test (Important!)

```cmd
# Test in isolated directory
test-release-standalone.bat
cd test-standalone-release
CellAnalyzer.exe
```

This verifies that:
- All DLLs are present
- ONNX models are accessible
- Application runs without dependencies on build environment

### Critical Files Checklist

- [ ] `CellAnalyzer.exe` exists
- [ ] `opencv_world4110.dll` exists
- [ ] `ml-data/models/yolov8s_cells_v1.0.onnx` exists (~25 MB)
- [ ] `ml-data/models/yolov8s_cells_v2.0.onnx` exists (~25 MB)
- [ ] Qt plugins folder (`platforms/`) exists
- [ ] Application starts without errors

## Common Issues

### "VCRUNTIME140.dll not found"

Install **Microsoft Visual C++ Redistributable**:
- Download: https://aka.ms/vs/17/release/vc_redist.x64.exe

### "Failed to load ONNX model"

Verify:
1. `ml-data/models/*.onnx` files are in correct location
2. Files are not corrupted (check size ~25 MB each)
3. Run from Release directory, not build root

### "Qt platform plugin could not be initialized"

Check:
1. `platforms/` folder exists in Release directory
2. `qwindows.dll` is present in `platforms/`
3. Run `windeployqt` again if needed

### CMake configuration fails

Ensure:
1. Visual Studio 2022 is installed
2. Qt is in PATH or specify: `-DCMAKE_PREFIX_PATH=C:/Qt/6.x.x/msvc2022_64`
3. CMake version is 3.16 or higher

## Distribution

### Creating Portable Package

1. Build release: `build-release.bat`
2. Test standalone: `test-release-standalone.bat`
3. If all checks pass, ZIP the `test-standalone-release` folder
4. Users can extract and run anywhere

### ZIP Archive (PowerShell)

```powershell
# Using build script
.\build-release.ps1
# Answer 'y' when prompted to create ZIP

# Or manually
Compress-Archive -Path "build-release\Release\*" -DestinationPath "CellAnalyzer-Release.zip"
```

## Development Build

For development with debugging:

```cmd
mkdir build-debug
cd build-debug
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Debug ..
cmake --build . --config Debug
```

## Continuous Integration (CI)

For automated builds (GitHub Actions, etc.):

```yaml
# Example GitHub Actions workflow
- name: Build Release
  run: |
    mkdir build-release
    cd build-release
    cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
    cmake --build . --config Release
    windeployqt --release Release/CellAnalyzer.exe
    # ... copy additional files
```

## Tips

1. **Always test standalone package** before distribution
2. **Check file sizes**: ONNX models should be ~25 MB each
3. **Use PowerShell script** for cleaner output and error handling
4. **Run from VS Developer Command Prompt** for manual builds
5. **Keep OpenCV path consistent** across team (or make configurable)

## Support

If you encounter build issues:
1. Check this document first
2. Verify all prerequisites are installed
3. Review CMakeLists.txt for path configurations
4. Open an issue with full build log
