# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CellAnalyzer is a Qt6-based desktop application for automated cell detection and analysis in microscopic images. It uses OpenCV for image processing and provides an intuitive GUI for researchers to measure cell diameters automatically, replacing manual measurement work.

### Main Features
- **Multiple Detection Algorithms**: HoughCircles, ContourBased, Watershed, Morphological, Adaptive Threshold, Blob Detection
- **Interactive Parameter Tuning**: Real-time preview with automatic parameter optimization
- **Advanced Statistics**: Comprehensive statistical analysis with outlier detection, distribution analysis, and correlation
- **Theme Support**: Light and dark themes with persistent settings
- **Zoomable Image Viewer**: Pan and zoom functionality for detailed inspection
- **Artifact Filtering**: Filter water droplets and other interference
- **Automatic Scale Bar Detection**: Calculate measurements in micrometers
- **Manual Verification**: Review and remove false positives
- **CSV Export**: Complete measurement data export
- **Individual Cell Image Extraction**: Save detected cells as separate images
- **Preset Management**: Save/load parameter presets with coefficient storage
- **Drag-and-Drop Support**: Easy image loading with improved preview grid

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

**AdvancedDetector** (`advanceddetector.h/cpp`)
- Multiple detection algorithms support:
  - HoughCircles (circular cells detection)
  - ContourBased (arbitrary shape detection)
  - WatershedSegmentation (overlapping cells separation)
  - MorphologicalOperations (shape-based detection)
  - AdaptiveThreshold (contrast-adaptive detection)
  - BlobDetection (keypoint-based detection)
- Per-algorithm parameter configuration
- Filtering by area, circularity, and other morphological properties
- Overlap removal and quality filtering

**AlgorithmSelectionWidget** (`algorithmselectionwidget.h/cpp`)
- UI for selecting detection algorithm
- Stacked parameter panels for each algorithm type
- Real-time parameter adjustment with preview
- Algorithm description tooltips
- Preset management integration

**StatisticsAnalyzer** (`statisticsanalyzer.h/cpp`)
- Comprehensive statistical analysis:
  - Basic statistics: mean, median, std deviation, variance, min, max, range
  - Quartiles: Q1, Q3, IQR
  - Advanced metrics: skewness, kurtosis, coefficient of variation
  - Distribution analysis with histogram binning
- Outlier detection:
  - IQR method (Interquartile Range)
  - Z-Score method (standard deviations)
- Correlation analysis (Pearson and Spearman)
- Grouping statistics by image
- Report generation (Text, CSV, Markdown formats)

**StatisticsWidget** (`statisticswidget.h/cpp`)
- Multi-tab statistics display:
  - Overview: Summary statistics and cell counts
  - Details: Per-parameter detailed statistics
  - Distribution: Histogram and distribution shape analysis
  - Correlation: Parameter correlation matrix
  - Outliers: Detected anomalies with filtering options
- Export functionality for statistical reports
- Table-based presentation with formatting
- Image group comparison

**ThemeManager** (`thememanager.h/cpp`)
- Singleton theme management system
- Two themes: Light and Dark
- Dynamic stylesheet generation
- Persistent theme settings
- Application-wide theme switching
- Custom styling for buttons, scrollbars, and widgets

**ZoomableImageWidget** (`zoomableimagewidget.h/cpp`)
- Advanced image viewing capabilities:
  - Mouse wheel zoom (10%-500%)
  - Pan with mouse drag
  - Zoom controls: buttons, slider, spin box
  - Fit to window function
  - Reset zoom to 100%
- Coordinate tracking and display
- Image size information
- Toolbar with zoom actions

**Logger** (`logger.h`)
- Thread-safe logging system with QMutex
- Log levels: DEBUG, INFO, WARNING, ERROR, CRITICAL
- Automatic log file rotation by size (default 10MB)
- Backup file management (default 5 backups)
- File and line number tracking in debug builds
- Console output in debug mode
- Convenient macros: LOG_DEBUG, LOG_INFO, LOG_WARNING, LOG_ERROR, LOG_CRITICAL

**ProgressDialog** (`progressdialog.h/cpp`)
- Two modes: indeterminate and determinate progress
- Animated loading indicator
- Elapsed time display
- Estimated time calculation
- Log message accumulation
- Cancel button with signal emission
- Auto-update timer for smooth progress

### Supporting Components

- **Cell** (`cell.h`): Enhanced cell data structure with center coordinates, radius, diameter (pixels/micrometers), area, image data, and deep copy support
- **ImprovedPreviewGrid** (`improvedpreviewgrid.h`): Advanced image grid with drag-and-drop, multi-selection, sorting (name/date/size), resizable thumbnails, context menus, and animations
- **PreviewGrid** (`previewgrid.h`): Basic grid for image thumbnails with adjustable size (100-500px)
- **CellItem/CellItemWidget** (`cellitem.h/cellitemwidget.h`): Data structure and widget for individual cell display
- **Utils** (`utils.h/cpp`): Helper functions including circle visibility calculations

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
   - Multiple detection algorithms (HoughCircles, Contours, Watershed, etc.)
   - Visibility filtering (minimum 60% of circle must be visible)
   - Scale detection using HoughLinesP
   - Morphological filtering and overlap removal
   - Unicode path support for international characters

2. **Responsive UI**:
   - QScrollArea for content overflow
   - Dynamic grid layouts that adjust to window size
   - Minimum window size: 800x600
   - Progress dialog with time estimation
   - Drag-and-drop support for images
   - Zoomable image preview with pan controls

3. **File Output**:
   - CSV export with columns: filename, cell_number, center_x, center_y, diameter_pixels, diameter_nm (micrometers)
   - Individual cell images saved separately
   - Statistical reports in Text/CSV/Markdown formats
   - Debug images with highlighted cells (when logging enabled)

4. **Error Handling**:
   - OpenCV exception catching
   - Boundary validation for drawing operations
   - User warnings for no detected cells
   - Thread-safe logging system with rotation
   - Comprehensive error logging with file/line info

5. **Styling and Theming**:
   - Light and Dark theme support
   - All buttons use border-radius: 10px
   - Light theme colors: Primary (#2196F3), Clear (#f44336), Add (#4CAF50), Analyze (#03A9F4)
   - Dark theme colors: adjusted for visibility and contrast
   - Hover effects on interactive elements
   - Adaptive button hover states
   - Custom scrollbar styling

6. **Performance Optimization**:
   - Lazy loading for image previews
   - Efficient cv::Mat deep copying in Cell structure
   - Log file rotation to prevent excessive disk usage
   - Configurable preview size for performance tuning
   - Batch processing with progress tracking

7. **Data Persistence**:
   - Settings saved to JSON format
   - Last-used preset auto-loading
   - Theme preference persistence
   - Window size and position memory
   - Parameter history tracking

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
3. View comprehensive statistics (optional)
4. Export to CSV with complete measurements
5. Save individual cell images
6. Export statistical reports (Text/CSV/Markdown)

### Statistics Analysis Workflow
1. After verification, access statistics widget
2. Review multi-tab analysis:
   - **Overview**: Summary statistics and counts
   - **Details**: Per-image group statistics
   - **Distribution**: Histogram and shape analysis
   - **Correlation**: Parameter relationships
   - **Outliers**: Anomaly detection and filtering
3. Export statistical reports in preferred format
4. Return to verification for additional cell filtering if needed

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