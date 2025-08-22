# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CellAnalyzer is a Qt6-based desktop application for automated cell detection and analysis in microscopic images. It uses OpenCV for image processing and provides an intuitive GUI for researchers to measure cell diameters automatically, replacing manual measurement work.

### Main Features
- Automatic detection of circular cells in microscopic images
- Interactive parameter tuning with real-time preview
- Artifact filtering (water droplets and other interference)
- Automatic scale bar detection on images
- Cell diameter calculation in pixels and nanometers
- Manual verification of detected cells
- CSV export of results
- Individual cell image extraction
- Save/load parameter presets for different image types

## Build Commands

### Building the project:
```bash
# Clean build (Windows)
build-release.bat

# Or using CMake directly:
mkdir build-release
cd build-release
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw Release\CellAnalyzer.exe

# Copy OpenCV DLLs (paths from build script):
copy "D:\opencv\build\x64\vc16\bin\opencv_world4110.dll" "Release\"
copy "D:\opencv\build\x64\vc16\bin\opencv_videoio_msmf4110_64.dll" "Release\"
copy "D:\opencv\build\x64\vc16\bin\opencv_videoio_ffmpeg4110_64.dll" "Release\"
```

### Running the application:
```bash
test_run.bat
# Or directly:
build\Release\CellAnalyzer.exe
```

## Architecture

### Core Components

**MainWindow** (`mainwindow.h/cpp`)
- Central application controller managing workflow states
- Handles transitions between initial state (no images) and working state (with images)
- Contains toolbar with preview size slider, clear button, add images button, and analyze button

**ImageProcessor** (`imageprocessor.h/cpp`)
- Core image processing logic using OpenCV HoughCircles algorithm
- Processes multiple images with configurable parameters
- Detects scale ruler and calculates nm/pixel ratio
- HoughCircles parameters:
  - dp = 1.0 (accumulator resolution)
  - minDist = 30.0 (minimum distance between circle centers)
  - param1 = 90.0 (Canny edge detector threshold)
  - param2 = 50.0 (circle center threshold)
  - minRadius = 30 pixels
  - maxRadius = 150 pixels

**ParameterTuningWidget** (`parametertuningwidget.h/cpp`)
- Interactive parameter tuning interface with controlled preview workflow
- **NEW**: Shows original image initially (no auto-detection)
- **NEW**: Interactive cell selection by clicking on image (red dots)
- **NEW**: "Применить параметры" button triggers detection preview
- **NEW**: Automatic parameter optimization based on selected cells
- **NEW**: Iterative optimization algorithm with 1-minute timeout
- **NEW**: Fixed coordinate mapping for accurate cell selection
- **NEW**: Stable image scaling (800x600 max) to prevent zoom issues
- **NEW**: Separate "Продолжить" button enabled only after applying parameters
- **NEW**: Overlap filtering with minDist parameter enforcement
- **NEW**: Smart evaluation algorithm ensuring each user point matches only one circle
- **NEW**: Complete reset functionality ("🔄 Сбросить всё" button)
- **NEW**: Unicode path support for images with Cyrillic characters
- **NEW**: Auto-apply parameters with 200ms delay when values change
- **NEW**: Guaranteed coverage algorithm using multiple search strategies
- **NEW**: Enhanced UI with bottom toolbar (Back, Reset All, Continue buttons)  
- **NEW**: Strategy-based optimization covering aggressive detection, threshold variation, and small circles
- **NEW**: Advanced preset system with coefficient storage and auto-application
- **NEW**: Preset management with save/delete functionality and last-used memory
- **NEW**: Workflow optimization for repeated use with template-based processing
- Allows saving/loading parameter presets with coefficients
- Uses sophisticated scoring system to evaluate parameter quality for selected cells

**VerificationWidget** (`verificationwidget.h/cpp`)
- Grid display of detected cells for manual verification
- Shows diameter in pixels and nanometers
- Allows removing false positives
- Exports results to CSV format

**SettingsManager** (`settingsmanager.h/cpp`)
- Singleton pattern for centralized settings management
- Saves to `AppData/Local/CellAnalyzer/settings.json`
- Auto-saves on parameter changes

### Supporting Components

- **PreviewGrid**: Dynamic grid for image thumbnails with adjustable size (100-500px)
- **CellItem/CellItemWidget**: Data structure and widget for individual cell display
- **Utils**: Helper functions including circle visibility calculations

### Key Design Patterns

1. **State Management**: MainWindow manages UI states (initial vs working)
2. **Singleton Pattern**: SettingsManager for global settings access
3. **Observer Pattern**: Qt signals/slots for component communication
4. **Factory Pattern**: Dynamic widget creation in preview and verification grids

## Dependencies

- **Qt6**: Widgets, Gui modules
- **OpenCV 4.11.0**: core, imgproc, highgui, objdetect modules
- **Compiler**: MSVC 2022
- **Build System**: CMake 3.16+
- **Platform**: Windows x64

## Important Implementation Details

1. **Image Processing Pipeline**:
   - Median blur preprocessing (`cv::medianBlur`)
   - HoughCircles for circle detection
   - Visibility filtering (minimum 60% of circle must be visible)
   - Scale detection using HoughLinesP
   - Scale text recognition (OCR placeholder for future implementation)

2. **Responsive UI**:
   - QScrollArea for content overflow
   - Dynamic grid layouts that adjust to window size
   - Minimum window size: 800x600
   - Progress bar during batch processing

3. **File Output**:
   - CSV export with columns: filename, cell_number, center_x, center_y, diameter_pixels, diameter_nm
   - Individual cell images saved separately
   - Debug images with highlighted cells (when logging enabled)

4. **Error Handling**:
   - OpenCV exception catching
   - Boundary validation for drawing operations
   - User warnings for no detected cells

5. **Styling**:
   - All buttons use border-radius: 10px
   - Color scheme: Primary (#2196F3), Clear (#f44336), Add (#4CAF50), Analyze (#03A9F4)
   - Hover effects on interactive elements
   - Adaptive button hover states

## User Workflows

### Single Image Analysis
1. User selects one image → Parameter tuning screen opens (shows original image without detection)
2. **NEW**: User can click on image to select target cells (red dots appear at click locations)
3. User can either:
   - Manually adjust parameters and click "Применить параметры" to see detection results
   - Load a saved preset and apply it
   - **NEW**: Select cells and use "🎯 Подобрать параметры" for automatic optimization
4. Once parameters are applied, preview shows both detected circles and selected cells
5. User can save parameter preset with custom name
6. **NEW**: "Продолжить" button (bottom-right) is enabled only after parameters are applied

### Batch Processing
1. User selects multiple images → Uses saved presets
2. Progress bar shows processing status
3. Automatic scale detection on each image
4. Cell detection using selected parameters

### Verification and Export
1. Grid view shows all detected cells
2. Click to remove false positives
3. Export to CSV with complete measurements
4. Save individual cell images

### **NEW: Optimized Template Workflow**
**First-time setup (detailed configuration):**
1. Load images → Parameter tuning page (shows original image)
2. Click on target cells to mark them (red dots)
3. Use "🎯 Подобрать параметры" for automatic optimization
4. Save preset with meaningful name (includes coefficient)
5. Continue to verification → Diameters auto-filled if coefficient available
6. Manual verification and CSV export

**Subsequent use (streamlined):**
1. Load images → Parameter tuning page  
2. Last-used preset auto-selected and applied
3. Verify correct preset is chosen → Continue
4. Verification page with pre-filled diameters
5. Quick verification and export

This workflow enables **one-time careful setup**, then **fast repeated processing** with the same image types.

## ToDo plan
1. all todo plan in file ./todo.md