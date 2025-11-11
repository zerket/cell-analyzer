# YOLOv8 Cell Detection Model - Output Format

**Model Version:** v1.0
**Model File:** `models/yolov8s_cells_v1.0.pt`
**Training Date:** 2025-11-11
**Training Dataset:** 100 images, 5,717 cells

## Model Performance

- **Box mAP50:** 0.993
- **Mask mAP50:** 0.993
- **Precision:** 0.970
- **Recall:** 0.977

## Model Output Structure

### Python API (Ultralytics)

```python
from ultralytics import YOLO

# Load model
model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')

# Run inference
results = model.predict(
    source='path/to/image.jpg',
    conf=0.25,      # Confidence threshold
    iou=0.7,        # NMS IoU threshold
    device=0        # GPU device (or 'cpu')
)

# Get first result
result = results[0]
```

### Output Format

For each detected cell, the model returns:

#### 1. Bounding Box
```python
boxes = result.boxes.data  # Shape: [N, 6]
# Each box: [x1, y1, x2, y2, confidence, class_id]

for box in boxes:
    x1, y1, x2, y2, conf, cls = box.cpu().numpy()
    # x1, y1: top-left corner
    # x2, y2: bottom-right corner
    # conf: confidence score (0.0 - 1.0)
    # cls: class ID (0 = cell, 1 = droplet)
```

#### 2. Segmentation Mask
```python
masks = result.masks.data  # Shape: [N, H, W]
# Each mask: binary matrix matching image dimensions

for i, mask in enumerate(masks):
    mask_np = mask.cpu().numpy()  # Shape: [H, W]
    # Values: 0.0 = background, 1.0 = cell

    # Calculate area
    area = mask_np.sum()  # pixels

    # Filter by area (remove water droplets)
    if area >= 500:  # minimum cell size
        # This is a cell
        pass
```

#### 3. Class Information
```python
classes = result.boxes.cls  # Shape: [N]
names = model.names  # {0: 'cell', 1: 'droplet'}

for cls_id in classes:
    class_name = names[int(cls_id)]
    print(f"Class: {class_name}")
```

### Complete Example

```python
from ultralytics import YOLO
import cv2
import numpy as np

# Load model
model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')

# Predict
image_path = 'path/to/image.jpg'
results = model.predict(source=image_path, conf=0.25, iou=0.7, device=0)
result = results[0]

# Get image dimensions
img = cv2.imread(image_path)
height, width = img.shape[:2]

# Process detections
cells = []
for i, (mask, box, cls) in enumerate(zip(result.masks.data, result.boxes.data, result.boxes.cls)):
    # Get mask
    mask_np = mask.cpu().numpy()
    area = mask_np.sum()

    # Filter by area (cells >= 500 pixels)
    if area < 500:
        continue  # Skip water droplets

    # Get bounding box
    x1, y1, x2, y2, conf, _ = box.cpu().numpy()

    # Resize mask to image size
    mask_resized = cv2.resize(mask_np, (width, height))
    mask_binary = (mask_resized > 0.5).astype(np.uint8)

    # Store cell data
    cell = {
        'id': i,
        'bbox': [float(x1), float(y1), float(x2), float(y2)],
        'confidence': float(conf),
        'area': float(area),
        'mask': mask_binary,  # Binary mask [H, W]
        'class': 'cell'
    }
    cells.append(cell)

print(f"Detected {len(cells)} cells")
```

## Qt/C++ Integration

### Option 1: Python Bridge (Recommended)

Use PyQt or PySide to call the Python model from Qt:

```cpp
// Qt C++ side
QProcess pythonProcess;
pythonProcess.start("python", QStringList() << "predict.py" << imagePath);
pythonProcess.waitForFinished();
QString output = pythonProcess.readAllStandardOutput();
// Parse JSON output
```

### Option 2: ONNX Export

Export model to ONNX format for C++ inference:

```python
from ultralytics import YOLO

model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
model.export(format='onnx', simplify=True)
# Creates: yolov8s_cells_v1.0.onnx
```

Then use ONNX Runtime in C++:
```cpp
#include <onnxruntime/core/session/onnxruntime_cxx_api.h>

// Load ONNX model
Ort::Env env;
Ort::Session session(env, L"yolov8s_cells_v1.0.onnx", Ort::SessionOptions{});

// Run inference...
```

### Option 3: TorchScript Export

Export to TorchScript for LibTorch C++ API:

```python
model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
model.export(format='torchscript')
```

## Output Conversion Scripts

### Convert to Simple Circles (for UI display)

```python
import cv2
import numpy as np

def bbox_to_circle(x1, y1, x2, y2, num_points=64):
    """Convert bbox to inscribed circle in square."""
    width = x2 - x1
    height = y2 - y1
    square_size = max(width, height)

    # Center and radius
    center_x = x1 + square_size / 2
    center_y = y1 + square_size / 2
    radius = square_size / 2

    # Generate circle points
    angles = np.linspace(0, 2 * np.pi, num_points, endpoint=False)
    points = []
    for angle in angles:
        x = center_x + radius * np.cos(angle)
        y = center_y + radius * np.sin(angle)
        points.append((int(x), int(y)))

    return points

# Usage
x1, y1, x2, y2 = box[:4]
circle_points = bbox_to_circle(x1, y1, x2, y2)

# Draw on Qt image
for i in range(len(circle_points)):
    p1 = circle_points[i]
    p2 = circle_points[(i + 1) % len(circle_points)]
    # painter.drawLine(p1[0], p1[1], p2[0], p2[1])
```

## Model Input Requirements

- **Image Format:** JPG, PNG
- **Image Size:** Any (model auto-resizes to 640x640 internally)
- **Color Space:** RGB or BGR (OpenCV uses BGR by default)
- **Recommended:** 640x640 to 2048x2048 pixels

## Model Classes

- **Class 0:** `cell` - Cell object
- **Class 1:** `droplet` - Water droplet (filter out with area < 500)

## Filtering Recommendations

```python
# Minimum cell area
MIN_CELL_AREA = 500  # pixels

# Confidence threshold
MIN_CONFIDENCE = 0.25

# Filter cells
valid_cells = []
for cell in detected_cells:
    if cell['area'] >= MIN_CELL_AREA and cell['confidence'] >= MIN_CONFIDENCE:
        valid_cells.append(cell)
```

## Performance

- **GPU (GTX 1060):** ~3-7 images/second
- **CPU:** ~0.5-1 images/second
- **Memory:** ~2 GB GPU VRAM

## Troubleshooting

### Issue: Too many false positives (water droplets)
**Solution:** Increase `MIN_CELL_AREA` or `MIN_CONFIDENCE`

### Issue: Missing cells
**Solution:** Decrease `conf` threshold in `model.predict()`

### Issue: Overlapping detections
**Solution:** Adjust `iou` threshold (higher = more aggressive NMS)

## Model Updates

To retrain/fine-tune the model, see `TRAINING.md`.

## Support

For issues or questions, see training logs:
- Training results: `instance_segmentation/checkpoints/yolov8s_cells_subset100_finetuned/`
- Metrics CSV: `instance_segmentation/checkpoints/yolov8s_cells_subset100_finetuned/results.csv`
