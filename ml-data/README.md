# Cell Analyzer - ML Data

Machine learning models and training scripts for cell detection in microscopy images.

## 📁 Directory Structure

```
cell-analyzer/ml-data/
├── models/
│   └── yolov8s_cells_v1.0.pt          # Current production model
├── scripts/
│   ├── convert_coco_to_yolo.py        # Convert CVAT annotations to YOLO
│   └── predict_simple_circles.py      # Generate predictions (simple circles)
├── docs/
│   ├── MODEL_OUTPUT.md                # Model output format for Qt integration
│   └── TRAINING.md                    # Training and fine-tuning guide
└── training_data/                     # Training datasets (created during training)
    └── (your datasets go here)
```

## 🚀 Quick Start

### 1. Load Model in Python

```python
from ultralytics import YOLO

model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
results = model.predict(source='image.jpg', conf=0.25, device=0)
```

### 2. Generate Predictions for CVAT

```bash
python cell-analyzer/ml-data/scripts/predict_simple_circles.py \
    --model cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
    --images /path/to/images \
    --output predictions.json
```

### 3. Fine-tune Model with New Data

See detailed instructions in [`docs/TRAINING.md`](docs/TRAINING.md)

## 📊 Current Model (v1.0)

**Performance:**
- **mAP50:** 0.993 (box and mask)
- **Precision:** 0.970
- **Recall:** 0.977

**Training Data:**
- 100 images
- 5,717 cells
- Trained: 2025-11-11

**Classes:**
- Class 0: `cell` (area >= 500 pixels)
- Class 1: `droplet` (area < 500 pixels, filtered out)

## 🔄 Active Learning Workflow

1. **Generate predictions** for new images
2. **Import to CVAT** and correct annotations
3. **Export** corrected annotations (COCO format)
4. **Convert** to YOLO format
5. **Fine-tune** model on corrected data
6. **Deploy** updated model
7. **Repeat** with more images

## 📖 Documentation

- **[MODEL_OUTPUT.md](docs/MODEL_OUTPUT.md)** - Complete model output specification
  - Python API usage
  - Output data structures
  - Qt/C++ integration options
  - Filtering recommendations

- **[TRAINING.md](docs/TRAINING.md)** - Training and fine-tuning guide
  - Step-by-step training workflow
  - Active learning cycles
  - Parameter tuning
  - Troubleshooting

## 🛠️ Requirements

```bash
pip install ultralytics opencv-python numpy tqdm torch torchvision
```

## 💻 Qt Integration

See [`docs/MODEL_OUTPUT.md`](docs/MODEL_OUTPUT.md) for three integration options:

1. **Python Bridge** (recommended) - Use PyQt/PySide
2. **ONNX Export** - Use ONNX Runtime in C++
3. **TorchScript** - Use LibTorch C++ API

### Example: Python Bridge

```python
# predict.py
import sys
import json
from ultralytics import YOLO

model = YOLO('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
results = model.predict(source=sys.argv[1], conf=0.25, device=0)

# Process and output JSON
cells = []
for result in results:
    for i, (mask, box) in enumerate(zip(result.masks.data, result.boxes.data)):
        x1, y1, x2, y2, conf, _ = box.cpu().numpy()
        area = mask.cpu().numpy().sum()

        if area >= 500:  # Filter cells
            cells.append({
                'id': i,
                'bbox': [float(x1), float(y1), float(x2), float(y2)],
                'confidence': float(conf),
                'area': float(area)
            })

print(json.dumps(cells))
```

```cpp
// Qt C++ side
QProcess python;
python.start("python", {"predict.py", imagePath});
python.waitForFinished();
QString output = python.readAllStandardOutput();
QJsonDocument json = QJsonDocument::fromJson(output.toUtf8());
// Process JSON...
```

## 🎯 Next Steps

### Immediate
- Correct annotations for subset_200 in CVAT
- Fine-tune model → v2.0
- Test in Qt Cell Analyzer

### Future
- Train on subset_500 → v3.0
- Train on subset_1000 → v4.0
- Train on full dataset (2726 images) → v5.0

## 📝 Model Versioning

| Version | Dataset | Images | Cells | mAP50 | Date |
|---------|---------|--------|-------|-------|------|
| v1.0 | subset_100 | 100 | 5,717 | 0.993 | 2025-11-11 |
| v2.0 | subset_200 | 200 | ~12,000 | TBD | Planned |
| v3.0 | subset_500 | 500 | ~30,000 | TBD | Future |

## 🐛 Troubleshooting

### Model not loading
```python
# Check model file exists
import os
assert os.path.exists('cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt')
```

### Out of memory
```bash
# Use CPU instead of GPU
python predict.py --device cpu
```

### Too many false positives
```python
# Increase confidence threshold
model.predict(source='image.jpg', conf=0.5)  # Higher = fewer detections
```

## 📧 Support

For issues or questions:
1. Check documentation in `docs/`
2. Review training logs in `training_runs/`
3. See original training data in `../../instance_segmentation/`

## 📜 License

Model trained on proprietary microscopy data. For internal use only.
