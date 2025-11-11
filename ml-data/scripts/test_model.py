#!/usr/bin/env python3
"""
Quick test script to verify model is working correctly.
"""

import sys
from pathlib import Path
from ultralytics import YOLO
import cv2
import numpy as np


def test_model(model_path: str, test_image: str = None):
    """
    Test model loading and inference.

    Args:
        model_path: Path to model file
        test_image: Optional test image path
    """
    print("=" * 60)
    print("Cell Detection Model Test")
    print("=" * 60)

    # Check model exists
    model_file = Path(model_path)
    if not model_file.exists():
        print(f"❌ ERROR: Model not found at {model_path}")
        return False

    print(f"✓ Model file found: {model_file.name}")
    print(f"  Size: {model_file.stat().st_size / (1024*1024):.1f} MB")

    # Load model
    try:
        print("\nLoading model...")
        model = YOLO(model_path)
        print("✓ Model loaded successfully")
    except Exception as e:
        print(f"❌ ERROR loading model: {e}")
        return False

    # Print model info
    print(f"\nModel Information:")
    print(f"  Task: {model.task}")
    print(f"  Classes: {model.names}")

    # Test inference
    if test_image:
        test_file = Path(test_image)
        if not test_file.exists():
            print(f"\n⚠ WARNING: Test image not found: {test_image}")
            return True

        print(f"\nTesting inference on: {test_file.name}")

        try:
            # Run prediction
            results = model.predict(
                source=str(test_file),
                conf=0.25,
                iou=0.7,
                device='cpu',  # Use CPU for testing
                verbose=False
            )

            result = results[0]

            # Count detections
            if result.masks is None:
                num_cells = 0
            else:
                # Filter by area
                num_cells = 0
                for mask in result.masks.data:
                    area = mask.cpu().numpy().sum()
                    if area >= 500:
                        num_cells += 1

            print(f"✓ Inference successful")
            print(f"  Detected {num_cells} cells")

            # Visualize (optional)
            if num_cells > 0:
                img = cv2.imread(str(test_file))
                height, width = img.shape[:2]

                for i, (mask, box) in enumerate(zip(result.masks.data, result.boxes.data)):
                    mask_np = mask.cpu().numpy()
                    area = mask_np.sum()

                    if area < 500:
                        continue

                    # Draw bbox
                    x1, y1, x2, y2, conf, _ = box.cpu().numpy()
                    cv2.rectangle(img, (int(x1), int(y1)), (int(x2), int(y2)), (0, 255, 0), 2)

                    # Draw label
                    label = f"Cell {i+1}"
                    cv2.putText(img, label, (int(x1), int(y1)-5),
                               cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

                # Save result
                output_path = test_file.parent / f"{test_file.stem}_detected.jpg"
                cv2.imwrite(str(output_path), img)
                print(f"  Saved visualization: {output_path.name}")

        except Exception as e:
            print(f"❌ ERROR during inference: {e}")
            return False

    print("\n" + "=" * 60)
    print("✓ All tests passed!")
    print("=" * 60)
    return True


if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(description='Test cell detection model')
    parser.add_argument('--model', default='cell-analyzer/ml-data/models/yolov8s_cells_v1.0.pt',
                        help='Path to model file')
    parser.add_argument('--image', help='Optional test image path')

    args = parser.parse_args()

    success = test_model(args.model, args.image)
    sys.exit(0 if success else 1)
