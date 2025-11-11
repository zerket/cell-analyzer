#!/bin/bash
#
# Fine-tune cell detection model with new annotated data
#
# Usage:
#   ./finetune_model.sh <dataset_name> <epochs> [device]
#
# Example:
#   ./finetune_model.sh subset_200 100 0
#

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check arguments
if [ $# -lt 2 ]; then
    echo -e "${RED}Error: Missing required arguments${NC}"
    echo "Usage: $0 <dataset_name> <epochs> [device]"
    echo ""
    echo "Arguments:"
    echo "  dataset_name  - Name of dataset folder in training_data/"
    echo "  epochs        - Number of training epochs (e.g., 100)"
    echo "  device        - GPU device number (default: 0) or 'cpu'"
    echo ""
    echo "Example:"
    echo "  $0 subset_200 100 0"
    exit 1
fi

DATASET_NAME=$1
EPOCHS=$2
DEVICE=${3:-0}

# Paths
BASE_DIR="cell-analyzer/ml-data"
DATASET_DIR="$BASE_DIR/training_data/$DATASET_NAME"
CURRENT_MODEL="$BASE_DIR/models/yolov8s_cells_v1.0.pt"
OUTPUT_DIR="$BASE_DIR/training_runs"

# Check dataset exists
if [ ! -d "$DATASET_DIR" ]; then
    echo -e "${RED}Error: Dataset not found at $DATASET_DIR${NC}"
    echo "Please convert your COCO annotations first:"
    echo "  python $BASE_DIR/scripts/convert_coco_to_yolo.py \\"
    echo "    --coco-zip /path/to/annotations.zip \\"
    echo "    --images-dir /path/to/images \\"
    echo "    --output-dir $DATASET_DIR"
    exit 1
fi

# Check model exists
if [ ! -f "$CURRENT_MODEL" ]; then
    echo -e "${RED}Error: Model not found at $CURRENT_MODEL${NC}"
    exit 1
fi

# Print configuration
echo -e "${GREEN}=== Fine-tuning Configuration ===${NC}"
echo "Dataset:       $DATASET_NAME"
echo "Epochs:        $EPOCHS"
echo "Device:        $DEVICE"
echo "Base Model:    $CURRENT_MODEL"
echo "Output:        $OUTPUT_DIR"
echo ""

# Count images
TRAIN_COUNT=$(ls -1 $DATASET_DIR/images/train/*.jpg 2>/dev/null | wc -l)
VAL_COUNT=$(ls -1 $DATASET_DIR/images/val/*.jpg 2>/dev/null | wc -l)

echo -e "${YELLOW}Dataset Statistics:${NC}"
echo "Training images:   $TRAIN_COUNT"
echo "Validation images: $VAL_COUNT"
echo ""

# Ask for confirmation
read -p "Continue with fine-tuning? (y/n) " -n 1 -r
echo
if [[ ! $REPLY =~ ^[Yy]$ ]]; then
    echo "Aborted."
    exit 1
fi

# Determine next version number
VERSION=2.0
if [ -d "$OUTPUT_DIR/yolov8s_cells_v2.0_$DATASET_NAME" ]; then
    VERSION=2.1
fi
if [ -d "$OUTPUT_DIR/yolov8s_cells_v2.1_$DATASET_NAME" ]; then
    VERSION=2.2
fi

OUTPUT_NAME="yolov8s_cells_v${VERSION}_$DATASET_NAME"

echo -e "${GREEN}Starting fine-tuning...${NC}"
echo "Output name: $OUTPUT_NAME"
echo ""

# Run training
yolo segment train \
    data=$DATASET_DIR/dataset.yaml \
    model=$CURRENT_MODEL \
    epochs=$EPOCHS \
    imgsz=640 \
    batch=8 \
    device=$DEVICE \
    project=$OUTPUT_DIR \
    name=$OUTPUT_NAME \
    patience=20 \
    lr0=0.0005 \
    augment=True \
    degrees=180.0 \
    scale=0.5 \
    flipud=0.5 \
    fliplr=0.5 \
    mosaic=1.0 \
    copy_paste=0.3 \
    shear=0.0 \
    perspective=0.0

# Check if training succeeded
if [ $? -eq 0 ]; then
    echo ""
    echo -e "${GREEN}=== Training Complete ===${NC}"
    echo ""
    echo "Results saved to: $OUTPUT_DIR/$OUTPUT_NAME"
    echo ""
    echo "Best model: $OUTPUT_DIR/$OUTPUT_NAME/weights/best.pt"
    echo "Last model: $OUTPUT_DIR/$OUTPUT_NAME/weights/last.pt"
    echo ""
    echo "View results:"
    echo "  cat $OUTPUT_DIR/$OUTPUT_NAME/results.csv"
    echo ""
    echo "To deploy this model as the new production model:"
    echo "  cp $OUTPUT_DIR/$OUTPUT_NAME/weights/best.pt \\"
    echo "     $BASE_DIR/models/yolov8s_cells_v${VERSION}.pt"
else
    echo -e "${RED}Training failed!${NC}"
    exit 1
fi
