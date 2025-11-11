# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

CellAnalyzer is a Qt6-based desktop application for automated cell detection and analysis in microscopic images. It uses **YOLOv8** for AI-powered cell detection via Python integration, and OpenCV for image processing. The application provides an intuitive GUI for researchers to measure cell diameters automatically, replacing manual measurement work.

### Main Features
- **YOLO-based Cell Detection**: YOLOv8 instance segmentation for accurate cell detection
- **Python Integration**: Seamless Python-Qt bridge via QProcess for YOLO inference
- **Advanced Statistics**: Comprehensive statistical analysis including % < 50 μm and % > 100 μm metrics
- **Theme Support**: Light and dark themes with persistent settings
- **Zoomable Image Viewer**: Pan and zoom functionality for detailed inspection
- **Automatic Scale Bar Detection**: Calculate measurements in micrometers
- **Manual Verification**: Review and remove false positives with interactive UI
- **CSV Export**: Complete measurement data export with μm values
- **Individual Cell Image Extraction**: Save detected cells as separate images
- **Coefficient Management**: Persistent pixel-to-micrometer coefficient storage
- **Drag-and-Drop Support**: Easy image loading with improved preview grid
- **Multi-image Tab Interface**: Separate tabs for each analyzed image

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
- Core YOLO-based cell detection using Python script integration
- Executes `ml-data/scripts/detect_cells.py` via QProcess
- Processes multiple images with configurable YOLO parameters
- Detects scale ruler and calculates μm/pixel ratio using OpenCV
- YOLO parameters (YoloParams structure):
  - modelPath: Path to YOLO model (.pt file) - default: "ml-data/models/yolov8s_cells_v1.0.pt"
  - confThreshold: Confidence threshold - default: 0.25
  - iouThreshold: NMS IoU threshold - default: 0.7
  - minCellArea: Minimum cell area in pixels to filter droplets - default: 500
  - device: CUDA device ("0" for GPU, "cpu" for CPU)
- Converts YOLO bounding boxes to circles for visualization
- Supports Unicode image paths through QImage fallback

**VerificationWidget** (`verificationwidget.h/cpp`)
- **NEW DESIGN**: Multi-tab interface with per-image organization
- Tab-based file navigation showing cell count per image
- Split panel layout: cell list (left 25%) and preview with markup (right 75%)
- Interactive cell selection: click on list or image to select, right-click on image to remove
- Manual diameter input in micrometers for each cell
- Coefficient calculation and management:
  - Manual input of μm diameters for reference cells
  - "Пересчитать" button calculates average coefficient and applies to empty fields
  - Coefficient saved to SettingsManager for persistence
  - Auto-loads saved coefficient on startup
- Cell info panel showing position, radius, and diameter
- Export to CSV with filename, cell number, position, diameter (pixels and μm)
- Debug images with highlighted cells and annotations

**SettingsManager** (`settingsmanager.h/cpp`)
- Singleton pattern for centralized settings management
- Saves settings to `settings.json` next to executable
- Manages:
  - Preview size (default: 150px)
  - Statistics thresholds (min: 50.0 μm, max: 100.0 μm)
  - Pixel-to-micrometer coefficient (persistent across sessions)
  - Theme preference (Light/Dark)
- Auto-saves on any setting change
- **REMOVED**: Preset management (no longer needed with YOLO)

**StatisticsAnalyzer** (`statisticsanalyzer.h/cpp`)
- Comprehensive statistical analysis (all metrics in micrometers):
  - Basic statistics: mean, median, std deviation, variance, min, max, range
  - Quartiles: Q1, Q3, IQR
  - Advanced metrics: skewness, kurtosis, coefficient of variation
  - **NEW**: % < 50 μm and % > 100 μm with cell counts
  - Distribution analysis with histogram binning
- Outlier detection:
  - IQR method (Interquartile Range)
  - Z-Score method (standard deviations)
- Correlation analysis (Pearson and Spearman)
- Grouping statistics by image
- Report generation (Text, CSV, Markdown formats)

**StatisticsWidget** (`statisticswidget.h/cpp`)
- Multi-tab statistics display:
  - Overview: Summary statistics with median, mean, % < 50 μm, % > 100 μm
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

- **Qt6**: Widgets, Gui, Concurrent modules
- **OpenCV 4.11.0**: core, imgproc, highgui modules (for scale detection and image loading)
- **Python 3.x**: Required for YOLO inference
- **Python packages**: ultralytics, opencv-python, numpy, torch
- **Compiler**: MSVC 2022
- **Build System**: CMake 3.16+
- **Platform**: Windows x64

## Important Implementation Details

1. **YOLO Detection Pipeline**:
   - Python script (`ml-data/scripts/detect_cells.py`) executed via QProcess
   - YOLO model performs instance segmentation
   - Bounding boxes converted to circles for visualization compatibility
   - Droplet filtering based on area threshold (default: 500 pixels)
   - JSON output parsed by Qt application
   - Scale detection using OpenCV HoughLinesP
   - Unicode path support for international characters via QImage fallback

2. **Responsive UI**:
   - QScrollArea for content overflow
   - Dynamic grid layouts that adjust to window size
   - Minimum window size: 800x600
   - Progress dialog with time estimation
   - Drag-and-drop support for images
   - Zoomable image preview with pan controls

3. **File Output**:
   - CSV export with columns: filename, cell_number, center_x, center_y, diameter_pixels, diameter_um (micrometers)
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
   - Settings saved to JSON format next to executable
   - Coefficient (μm/pixel) persistence across sessions
   - Theme preference persistence
   - No presets needed - YOLO parameters are fixed

## User Workflows

### **NEW: Simplified YOLO Workflow**

**Step 1: Load Images**
1. User selects one or multiple images
2. Images displayed in preview grid
3. Click "Начать анализ (YOLO)" button

**Step 2: Automatic Detection**
1. YOLO model processes all images automatically
2. No parameter tuning needed - AI handles detection
3. Progress shown during processing
4. Results appear in verification widget

**Step 3: Verification and Coefficient**
1. **Multi-tab interface**: Separate tab for each analyzed image
2. **Split view**: Cell list (left) and image preview (right)
3. **Interactive selection**: Click cells in list or on image
4. **Coefficient workflow**:
   - Measure a few cells manually with microscope
   - Enter real μm values in the "Диаметр (мкм)" column
   - Click "Пересчитать" to calculate and apply coefficient to all cells
   - Coefficient auto-saves for future sessions
5. **Remove false positives**: Right-click on image or click ❌ button
6. **Export**: Click "💾 Сохранить" to export CSV with all measurements

**Step 4: Statistics (Optional)**
1. Click "📊 Статистика" button
2. Review multi-tab analysis:
   - **Overview**: Median, mean, % < 50 μm, % > 100 μm
   - **Details**: Full statistics per parameter
   - **Distribution**: Histogram and distribution analysis
   - **Correlation**: Parameter relationships
   - **Outliers**: Anomaly detection
3. Export statistical reports (Text/CSV/Markdown)
4. Return to verification for adjustments

### Key Advantages
- **No parameter tuning** - YOLO works automatically
- **Persistent coefficient** - Set once, use forever (for same microscope/magnification)
- **Fast workflow** - Load → Detect → Verify → Export
- **Accurate AI detection** - 99.3% mAP50 on trained data

## ToDo plan
1. all todo plan in file ./todo.md