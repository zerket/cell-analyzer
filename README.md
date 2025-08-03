# CellAnalyzer

A Qt6-based desktop application for automated cell detection and analysis in microscopic images.

## Features

- **Automatic Cell Detection**: Uses OpenCV's HoughCircles algorithm to detect circular cells
- **Interactive Parameter Tuning**: Real-time preview while adjusting detection parameters
- **Scale Bar Detection**: Automatically detects scale bars to calculate measurements in nanometers
- **Batch Processing**: Analyze multiple images with saved parameter presets
- **Manual Verification**: Review and remove false positives before exporting
- **Data Export**: Export results to CSV with cell measurements and individual cell images
- **Artifact Filtering**: Filter out water droplets and other interference

## Installation

### Prerequisites

- Windows x64
- Visual Studio 2022
- CMake 3.16+
- Qt6
- OpenCV 4.11.0

### Building from Source

```bash
# Clean build
build-release.bat

# Or manually:
mkdir build-release
cd build-release
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release
```

### Running the Application

```bash
test_run.bat
# Or directly:
build\Release\CellAnalyzer.exe
```

## Usage

1. **Load Images**: Click "Add Images" or drag and drop microscopic images
2. **Parameter Tuning** (for single images): Adjust detection parameters with real-time preview
3. **Batch Processing** (for multiple images): Select a saved parameter preset
4. **Verification**: Review detected cells and remove any false positives
5. **Export**: Save results as CSV and export individual cell images

## Output Format

The CSV export contains:
- Filename
- Cell number
- Center coordinates (X, Y)
- Diameter in pixels
- Diameter in nanometers

## License

[Add license information]

## Contributing

[Add contribution guidelines]