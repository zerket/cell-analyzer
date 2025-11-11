# Quick Start Guide

Get started with the cell detection model in 5 minutes.

## 1. Test the Model

```bash
# Basic test
python cell-analyzer/ml-data/scripts/test_model.py

# Test with your image
python cell-analyzer/ml-data/scripts/test_model.py \
    --image /path/to/your/image.jpg
```

## 2. Use Model in Python

```python
from ultralytics import YOLO

# Load model
model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')

# Predict
results = model.predict(source='image.jpg', conf=0.25, device=0)

# Get cells
for result in results:
    for i, (mask, box) in enumerate(zip(result.masks.data, result.boxes.data)):
        x1, y1, x2, y2, conf, _ = box.cpu().numpy()
        area = mask.cpu().numpy().sum()

        if area >= 500:  # Filter cells
            print(f"Cell {i}: bbox=({x1:.0f}, {y1:.0f}, {x2:.0f}, {y2:.0f}), "
                  f"confidence={conf:.2f}, area={area:.0f}")
```

## 3. Generate Predictions for CVAT

```bash
python cell-analyzer/ml-data/scripts/predict_simple_circles.py \
    --model cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
    --images /path/to/images/ \
    --output predictions.json \
    --device 0
```

Then import `predictions.json` into CVAT (COCO 1.0 format).

## 4. Fine-tune Model (After Correcting Annotations)

### Step 1: Convert CVAT annotations to YOLO format

```bash
python cell-analyzer/ml-data/scripts/convert_coco_to_yolo.py \
    --coco-zip /path/to/annotations_corrected.zip \
    --images-dir /path/to/images \
    --output-dir cell-analyzer/ml-data/training_data/my_dataset
```

### Step 2: Fine-tune model

```bash
# Linux/Mac
./cell-analyzer/ml-data/scripts/finetune_model.sh my_dataset 100 0

# Windows (or manual)
yolo segment train \
    data=cell-analyzer/ml-data/training_data/my_dataset/dataset.yaml \
    model=cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
    epochs=100 \
    batch=8 \
    device=0 \
    project=cell-analyzer/ml-data/training_runs \
    name=yolov8s_cells_v2.0
```

### Step 3: Deploy new model

```bash
# Backup old model
cp cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
   cell-analyzer/ml-data/models/yolov8s_cells_v1.0_backup.pt

# Deploy new model
cp cell-analyzer/ml-data/training_runs/yolov8s_cells_v2.0/weights/best.pt \
   cell-analyzer/ml-data/models/yolov8s_cells_v2.0.pt
```

## 5. Qt Integration

### Option A: Python Bridge (Recommended)

**Python script (predict.py):**
```python
import sys, json
from ultralytics import YOLO

model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
results = model.predict(source=sys.argv[1], conf=0.25, device=0)

cells = []
for result in results:
    for mask, box in zip(result.masks.data, result.boxes.data):
        x1, y1, x2, y2, conf, _ = box.cpu().numpy()
        area = mask.cpu().numpy().sum()
        if area >= 500:
            cells.append({
                'bbox': [float(x1), float(y1), float(x2), float(y2)],
                'confidence': float(conf)
            })

print(json.dumps({'cells': cells}))
```

**Qt C++ code:**
```cpp
// Run Python script
QProcess python;
python.start("python", {"predict.py", imagePath});
python.waitForFinished();

// Parse JSON output
QJsonDocument doc = QJsonDocument::fromJson(python.readAllStandardOutput());
QJsonArray cells = doc.object()["cells"].toArray();

// Process cells
for (const auto& cell : cells) {
    QJsonObject obj = cell.toObject();
    QJsonArray bbox = obj["bbox"].toArray();
    double x1 = bbox[0].toDouble();
    double y1 = bbox[1].toDouble();
    double x2 = bbox[2].toDouble();
    double y2 = bbox[3].toDouble();
    double conf = obj["confidence"].toDouble();

    // Draw rectangle or circle
    // ...
}
```

### Option B: ONNX Export

```python
# Export model
from ultralytics import YOLO
model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
model.export(format='onnx', simplify=True)
```

Then use ONNX Runtime in C++.

## Next Steps

- **Documentation:** See [`docs/MODEL_OUTPUT.md`](docs/MODEL_OUTPUT.md) for complete API reference
- **Training Guide:** See [`docs/TRAINING.md`](docs/TRAINING.md) for advanced training options
- **Full README:** See [`README.md`](README.md) for complete overview

## Common Issues

### Model not loading
```python
# Verify file exists
import os
assert os.path.exists('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
```

### Out of memory
```bash
# Use CPU
python predict.py --device cpu

# Or reduce batch size
yolo segment train ... batch=4
```

### Too many false positives
```python
# Increase confidence threshold
model.predict(source='image.jpg', conf=0.5)  # Default: 0.25
```

## Support

- Issues: Check [`docs/TRAINING.md`](docs/TRAINING.md) troubleshooting section
- Training logs: `cell-analyzer/ml-data/training_runs/`
- Original training: `instance_segmentation/`
