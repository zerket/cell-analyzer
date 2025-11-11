# Model Training and Fine-tuning Guide

This guide explains how to fine-tune the YOLOv8 cell detection model with new annotated data.

## Prerequisites

```bash
pip install ultralytics opencv-python numpy tqdm torch torchvision
```

## Directory Structure

```
cell-analyzer/ml-data/
├── models/
│   └── yolov8s_cells_v1.0.pt          # Current model
├── scripts/
│   ├── convert_coco_to_yolo.py         # Convert annotations
│   ├── train_yolov8.py                 # Training script
│   └── predict_simple_circles.py       # Inference script
├── docs/
│   ├── MODEL_OUTPUT.md                 # This file
│   └── TRAINING.md                     # Training guide
└── training_data/                      # Your training datasets
    ├── subset_100/
    ├── subset_200/
    └── ...
```

## Training Workflow

### Step 1: Prepare Annotations

Annotations should be in **COCO 1.0 format** exported from CVAT.

Example structure:
```
annotations_subset_200.zip
├── annotations/
│   └── instances_default.json
└── images/
    ├── image1.jpg
    ├── image2.jpg
    └── ...
```

### Step 2: Convert Annotations to YOLO Format

```bash
python cell-analyzer/ml-data/scripts/convert_coco_to_yolo.py \
    --coco-zip /path/to/annotations_subset_200.zip \
    --images-dir /path/to/images/subset_200 \
    --output-dir cell-analyzer/ml-data/training_data/subset_200
```

This creates:
```
training_data/subset_200/
├── dataset.yaml              # YOLO dataset config
├── images/
│   ├── train/               # 80% of images
│   └── val/                 # 20% of images
└── labels/
    ├── train/               # Corresponding labels
    └── val/
```

### Step 3: Fine-tune the Model

```bash
yolo segment train \
    data=cell-analyzer/ml-data/training_data/subset_200/dataset.yaml \
    model=cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
    epochs=100 \
    imgsz=640 \
    batch=8 \
    device=0 \
    project=cell-analyzer/ml-data/training_runs \
    name=yolov8s_cells_v2.0 \
    patience=20 \
    lr0=0.0005 \
    augment=True \
    degrees=180.0 \
    scale=0.5 \
    flipud=0.5 \
    fliplr=0.5 \
    mosaic=1.0 \
    copy_paste=0.3
```

**Parameters explained:**
- `data`: Path to dataset config
- `model`: Path to base model (current model for fine-tuning)
- `epochs`: Maximum training epochs
- `imgsz`: Image size (640x640)
- `batch`: Batch size (adjust based on GPU memory)
- `device`: GPU device (0, 1, ...) or 'cpu'
- `project`: Output directory
- `name`: Experiment name
- `patience`: Early stopping patience (stops if no improvement for N epochs)
- `lr0`: Initial learning rate
- `augment`: Enable augmentation
- `degrees`: Rotation augmentation (0-180)
- `scale`: Scale augmentation
- `mosaic`: Mosaic augmentation
- `copy_paste`: Copy-paste augmentation (good for cells)

### Step 4: Monitor Training

Training progress is saved in `cell-analyzer/ml-data/training_runs/yolov8s_cells_v2.0/`:

```bash
# View results
cat cell-analyzer/ml-data/training_runs/yolov8s_cells_v2.0/results.csv

# View training curves
# Open: cell-analyzer/ml-data/training_runs/yolov8s_cells_v2.0/results.png
```

**Key metrics to watch:**
- `metrics/mAP50(M)`: Mask mAP at 0.5 IoU (higher is better, aim for >0.99)
- `metrics/precision(M)`: Mask precision
- `metrics/recall(M)`: Mask recall
- `train/seg_loss`: Segmentation loss (should decrease)

### Step 5: Evaluate Model

```bash
yolo segment val \
    model=cell-analyzer/ml-data/training_runs/yolov8s_cells_v2.0/weights/best.pt \
    data=cell-analyzer/ml-data/training_data/subset_200/dataset.yaml \
    device=0
```

### Step 6: Test Predictions

```bash
python cell-analyzer/ml-data/scripts/predict_simple_circles.py \
    --model cell-analyzer/ml-data/training_runs/yolov8s_cells_v2.0/weights/best.pt \
    --images /path/to/test/images \
    --output predictions.json \
    --device 0
```

### Step 7: Deploy Updated Model

If results are satisfactory:

```bash
# Backup old model
mv cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
   cell-analyzer/ml-data/models/yolov8s_cells_v1.0_backup.pt

# Deploy new model
cp cell-analyzer/ml-data/training_runs/yolov8s_cells_v2.0/weights/best.pt \
   cell-analyzer/ml-data/models/yolov8s_cells_v2.0.pt
```

## Active Learning Workflow

This is the recommended workflow for iteratively improving the model:

### Cycle N (Example: 200 images)

1. **Generate predictions** for unlabeled images:
   ```bash
   python cell-analyzer/ml-data/scripts/predict_simple_circles.py \
       --model cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
       --images cell-data/iterative-image-subsets/subset_200 \
       --output predictions_subset_200.json
   ```

2. **Import to CVAT**:
   - Create new task in CVAT
   - Upload images from `subset_200`
   - Import annotations from `predictions_subset_200.json`

3. **Correct annotations** in CVAT:
   - Fix false positives (water droplets detected as cells)
   - Add missing cells
   - Adjust boundaries

4. **Export corrected annotations**:
   - Format: COCO 1.0
   - Save as: `annotations_subset_200_corrected.zip`

5. **Convert to YOLO**:
   ```bash
   python cell-analyzer/ml-data/scripts/convert_coco_to_yolo.py \
       --coco-zip annotations_subset_200_corrected.zip \
       --images-dir cell-data/iterative-image-subsets/subset_200 \
       --output-dir cell-analyzer/ml-data/training_data/subset_200
   ```

6. **Fine-tune model**:
   ```bash
   yolo segment train \
       data=cell-analyzer/ml-data/training_data/subset_200/dataset.yaml \
       model=cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt \
       epochs=100 \
       batch=8 \
       device=0 \
       project=cell-analyzer/ml-data/training_runs \
       name=yolov8s_cells_v2.0_subset200 \
       patience=20
   ```

7. **Repeat** with subset_500, subset_1000, etc.

## Training Parameters Guide

### GPU Memory Issues

If you get OOM (Out of Memory) errors:

```bash
# Reduce batch size
batch=4  # or even batch=2

# Reduce image size
imgsz=512  # instead of 640
```

### CPU Training

If no GPU available:

```bash
device=cpu
batch=2
imgsz=512
workers=4
```

### Augmentation Tuning

For better cell detection:

```python
# Recommended augmentation for circular cells
augment=True
degrees=180.0      # Full rotation (cells are rotationally symmetric)
scale=0.5          # Size variation
flipud=0.5         # Vertical flip
fliplr=0.5         # Horizontal flip
mosaic=1.0         # Mosaic augmentation
copy_paste=0.3     # Copy-paste (good for scattered cells)
shear=0.0          # No shear (cells are circular)
perspective=0.0    # No perspective (2D microscopy)
```

### Learning Rate

```bash
# For fine-tuning (default)
lr0=0.0005  # Lower LR for fine-tuning existing model

# For training from scratch
lr0=0.01    # Higher LR for new model
```

## Training History

### v1.0 (Current Model)
- **Dataset:** 100 images, 5,717 cells
- **Base model:** YOLOv8s-seg pretrained
- **Training epochs:** 73 (early stopped at epoch 57)
- **Best mAP50:** 0.993
- **Training time:** ~1.5 hours (GTX 1060)

### v2.0 (Planned)
- **Dataset:** 200 images (after corrections)
- **Base model:** v1.0
- **Expected mAP50:** >0.995

## Troubleshooting

### Issue: Model detects water droplets as cells
**Solution:**
- Add more water droplet examples to training data
- Use lower `conf` threshold during prediction
- Post-process with area filtering (area < 500 = droplet)

### Issue: Model misses small cells
**Solution:**
- Ensure small cells are annotated in training data
- Lower `conf` threshold
- Try `imgsz=1280` for higher resolution

### Issue: Overlapping cell boundaries
**Solution:**
- Increase `iou` threshold in NMS
- Annotate more examples with clear boundaries
- Check that annotations don't overlap

### Issue: Training is slow
**Solution:**
- Use smaller batch size won't help speed (but reduces memory)
- Use `imgsz=512` instead of 640
- Use fewer augmentations
- Use faster GPU

### Issue: Loss is not decreasing
**Solution:**
- Check dataset quality (corrupted images/labels)
- Increase learning rate `lr0=0.001`
- Train longer (increase `epochs`)
- Ensure enough training data (>50 images minimum)

## Best Practices

1. **Always backup your current model** before training a new one
2. **Use early stopping** (`patience=20`) to prevent overfitting
3. **Monitor validation metrics**, not just training loss
4. **Test on held-out images** before deploying
5. **Keep training logs** for reproducibility
6. **Version your models** (v1.0, v2.0, etc.)
7. **Document your dataset** (number of images, annotations, sources)

## Next Steps

After training v2.0 on 200 images:
1. Generate predictions for subset_500
2. Correct in CVAT
3. Train v3.0
4. Repeat until mAP50 > 0.995 and visual inspection is satisfactory

Target: Full dataset (2726 images) → Final model v5.0+
