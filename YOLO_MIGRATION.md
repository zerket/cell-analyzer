# CellAnalyzer - Migration to YOLO

## Overview

CellAnalyzer has been successfully migrated from HoughCircles-based detection to YOLOv8-based cell detection. This provides more accurate and robust cell detection using deep learning.

## What Changed

### ✅ Completed Changes

1. **Removed HoughCircles Detection**
   - All HoughCircles-based detection logic has been replaced with YOLO
   - Removed ParameterTuningWidget (no longer needed)
   - Removed manual parameter adjustment interface
   - Deleted unused files:
     - `parametertuningwidget.h/cpp`
     - `advanceddetector.h/cpp`
     - `algorithmselectionwidget.h/cpp`
     - `neuralnetdetector.h/cpp`
     - `neuralnetparameterswidget.h/cpp`
     - `markupimagewidget.h/cpp`

2. **Added YOLO Detection**
   - New Python script: `ml-data/scripts/detect_cells.py`
   - Automatically converts YOLO bboxes to circles (inscribed in square)
   - Edge-aware circle expansion logic (same as used in predictions)
   - Returns JSON with detected cells

3. **Updated ImageProcessor**
   - `imageprocessor.h`: New `YoloParams` structure
   - `imageprocessor.cpp`: Calls Python script via QProcess
   - Parses JSON results and creates Cell objects
   - Maintains scale detection for μm conversion

4. **Simplified MainWindow**
   - Removed parameter tuning workflow
   - Direct analysis: Select images → Analyze → Verify
   - No manual parameter adjustment needed
   - Updated title: "Cell Analyzer - Анализатор клеток (YOLO)"

5. **Circle Visualization**
   - VerificationWidget automatically displays circles
   - Uses Cell.circle field (cv::Vec3f) from YOLO detection
   - Circles inscribed in square bboxes as designed

## Architecture

### Detection Workflow

```
User selects images
    ↓
MainWindow::startAnalysis()
    ↓
ImageProcessor::processImages()
    ↓
ImageProcessor::detectCellsWithYolo()
    ↓
Calls: python3 detect_cells.py --model ... --image ...
    ↓
Python: YOLO detection → bbox to circle conversion → JSON output
    ↓
C++: Parse JSON → Create Cell objects
    ↓
VerificationWidget displays circles
```

### YOLO Detection Script

**Location**: `cell-analyzer/ml-data/scripts/detect_cells.py`

**Parameters**:
- `--model`: Path to YOLO model (.pt file)
- `--image`: Input image path
- `--conf`: Confidence threshold (default: 0.25)
- `--iou`: NMS IoU threshold (default: 0.7)
- `--min-area`: Minimum cell area to filter droplets (default: 500)
- `--device`: CUDA device or 'cpu' (default: '0')

**Output**: JSON with detected cells
```json
{
  "success": true,
  "num_cells": 42,
  "cells": [
    {
      "center_x": 123.4,
      "center_y": 567.8,
      "radius": 45.2,
      "diameter_pixels": 90.4,
      "area": 6420,
      "confidence": 0.95,
      "bbox": {"x1": 78.2, "y1": 522.6, "x2": 168.6, "y2": 613.0},
      "square_bbox": {"x": 78.2, "y": 522.6, "size": 90.4}
    },
    ...
  ]
}
```

### Circle Conversion Logic

The script uses the same edge-aware logic as `predict_simple_circles.py`:

1. Convert rectangular bbox to square (using max(width, height))
2. Determine anchor point based on edge proximity:
   - **Near left/top edges**: Expand from bottom-right corner
   - **Otherwise**: Expand from top-left corner
3. Inscribe circle in the square
4. **No clamping**: Circles can extend beyond image boundaries

This ensures cells at edges are properly sized even when partially visible.

## Configuration

### Default YOLO Parameters

```cpp
struct YoloParams {
    QString modelPath = "ml-data/models/yolov8s_cells_v1.0.pt";
    double confThreshold = 0.25;
    double iouThreshold = 0.7;
    int minCellArea = 500;
    QString device = "0";  // "0" for GPU, "cpu" for CPU
};
```

These can be adjusted in `ImageProcessor::startAnalysis()` if needed.

### Model Location

**Current model**: `cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt`

To update the model:
1. Train new model or fine-tune existing one
2. Copy best weights: `cp training_runs/.../weights/best.pt models/yolov8s_cells_v2.0.pt`
3. Update `YoloParams::modelPath` in code if needed

## Dependencies

### Python Requirements

```bash
pip3 install ultralytics opencv-python numpy
```

### Qt/C++ Requirements

- Qt6 (Widgets, Gui, Core)
- OpenCV 4.x
- QProcess for Python script execution
- QJson for parsing detection results

## Testing

### Quick Test

1. Build the application:
```bash
cd cell-analyzer
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
```

2. Run from build directory (ensures correct paths):
```bash
./CellAnalyzer
```

3. Test workflow:
   - Select images (e.g., from `cell-data/iterative-image-subsets/subset_20`)
   - Click "Начать анализ (YOLO)"
   - Verify cells are detected and displayed as circles
   - Check verification interface works correctly

### Expected Behavior

- **Model loading**: ~2-3 seconds on first run
- **Detection speed**: ~1-2 seconds per image (GPU) or ~10-20 seconds (CPU)
- **Circle visualization**: Circles inscribed in square bboxes, may extend beyond image edges
- **Scale detection**: Automatic μm conversion if scale bar present

## Troubleshooting

### "YOLO detection script not found"

Check paths in `ImageProcessor::detectCellsWithYolo()`:
- Script: `cell-analyzer/ml-data/scripts/detect_cells.py`
- Model: `cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt`

Make sure to run from correct directory or use absolute paths.

### "YOLO model not found"

Verify model file exists:
```bash
ls -lh cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt
```

### Python script fails

Test manually:
```bash
python3 cell-analyzer/ml-data/scripts/detect_cells.py \
    --model cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
    --image /path/to/test/image.jpg
```

Check output for errors.

### No cells detected

- Lower confidence threshold: Edit `YoloParams::confThreshold` (e.g., 0.15)
- Check image quality and cell visibility
- Review YOLO model performance on validation set

## Future Improvements

1. **Parameter UI**: Optional dialog to adjust conf/iou thresholds
2. **Batch processing optimization**: Process multiple images in parallel
3. **Model selection**: UI to choose between different trained models
4. **Real-time preview**: Show detection preview before full analysis
5. **GPU optimization**: Batch multiple images in single YOLO call

## Notes

- The application now requires Python 3 and ultralytics package
- Detection is slower than HoughCircles but much more accurate
- Circle visualization matches prediction format for consistency
- Scale detection still works for μm conversion
- All other features (verification, statistics, export) remain unchanged

## Migration Date

**Completed**: 2025-11-11
**YOLO Model Version**: yolov8s_cells_v1.0
**Trained on**: subset_100 (100 images, ~6000 annotated cells)
