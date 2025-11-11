#!/usr/bin/env python3
"""
YOLO Cell Detection Script for CellAnalyzer Qt Application
Called from C++ via QProcess, outputs JSON with detected cells
"""

import sys
import json
import argparse
from pathlib import Path
import cv2
import numpy as np
from ultralytics import YOLO


def bbox_to_circle(x1, y1, x2, y2, img_width, img_height):
    """
    Convert bbox to square and create inscribed circle.
    Returns circle parameters (center_x, center_y, radius).

    Args:
        x1, y1: top-left corner
        x2, y2: bottom-right corner
        img_width: image width
        img_height: image height

    Returns:
        tuple: (center_x, center_y, radius, square_x1, square_y1, square_size)
    """
    # Calculate bbox dimensions
    width = x2 - x1
    height = y2 - y1

    # Convert to square: use max(width, height)
    square_size = max(width, height)

    # Define "near edge" threshold (within 10% of image dimensions)
    edge_threshold_x = img_width * 0.1
    edge_threshold_y = img_height * 0.1

    # Check proximity to edges
    near_left = x1 < edge_threshold_x
    near_top = y1 < edge_threshold_y
    near_right = x2 > (img_width - edge_threshold_x)
    near_bottom = y2 > (img_height - edge_threshold_y)

    # Decide anchor point based on which edge the bbox is near
    if near_left or near_top:
        # Bbox near left or top edge → expand from bottom-right corner
        square_x1 = x2 - square_size
        square_y1 = y2 - square_size
        square_x2 = x2
        square_y2 = y2
    else:
        # Bbox near right or bottom edge (or in center) → expand from top-left corner
        square_x1 = x1
        square_y1 = y1
        square_x2 = x1 + square_size
        square_y2 = y1 + square_size

    # Circle inscribed in square
    center_x = square_x1 + square_size / 2
    center_y = square_y1 + square_size / 2
    radius = square_size / 2

    return (center_x, center_y, radius, square_x1, square_y1, square_size)


def detect_cells(model_path, image_path, conf_threshold=0.25, iou_threshold=0.7,
                 min_cell_area=500, device='0'):
    """
    Detect cells in image using YOLO model.

    Args:
        model_path: Path to YOLO model (.pt file)
        image_path: Path to input image
        conf_threshold: Confidence threshold for detection
        iou_threshold: NMS IoU threshold
        min_cell_area: Minimum cell area to filter droplets
        device: CUDA device or 'cpu'

    Returns:
        dict: Detection results in JSON format
    """
    # Load model
    model = YOLO(model_path)

    # Load image to get dimensions
    img = cv2.imread(image_path)
    if img is None:
        raise ValueError(f"Failed to load image: {image_path}")

    height, width = img.shape[:2]

    # Run prediction
    results = model.predict(
        source=image_path,
        conf=conf_threshold,
        iou=iou_threshold,
        device=device,
        verbose=False,
        save=False
    )

    result = results[0]

    cells = []

    if result.masks is not None:
        # Process each detected object
        for idx, (mask, box, cls) in enumerate(zip(result.masks.data, result.boxes.data, result.boxes.cls)):
            # Get mask area
            mask_np = mask.cpu().numpy()
            area = int(mask_np.sum())

            # Determine if cell or droplet
            is_droplet = area < min_cell_area
            if is_droplet:
                continue  # Skip droplets

            # Get bounding box from YOLO
            x1, y1, x2, y2, conf, _ = box.cpu().numpy()

            # Convert bbox to circle parameters
            center_x, center_y, radius, square_x1, square_y1, square_size = bbox_to_circle(
                x1, y1, x2, y2, width, height
            )

            # Create cell dict
            cell = {
                'center_x': float(center_x),
                'center_y': float(center_y),
                'radius': float(radius),
                'diameter_pixels': float(2 * radius),
                'area': area,
                'confidence': float(conf),
                'bbox': {
                    'x1': float(x1),
                    'y1': float(y1),
                    'x2': float(x2),
                    'y2': float(y2)
                },
                'square_bbox': {
                    'x': float(square_x1),
                    'y': float(square_y1),
                    'size': float(square_size)
                }
            }

            cells.append(cell)

    return {
        'success': True,
        'image_path': image_path,
        'image_width': width,
        'image_height': height,
        'num_cells': len(cells),
        'cells': cells
    }


def main():
    parser = argparse.ArgumentParser(description='YOLO Cell Detection for CellAnalyzer')
    parser.add_argument('--model', required=True, help='Path to YOLO model')
    parser.add_argument('--image', required=True, help='Path to input image')
    parser.add_argument('--conf', type=float, default=0.25, help='Confidence threshold')
    parser.add_argument('--iou', type=float, default=0.7, help='NMS IoU threshold')
    parser.add_argument('--min-area', type=int, default=500,
                        help='Minimum cell area (pixels) to filter droplets')
    parser.add_argument('--device', default='0', help='CUDA device or cpu')
    parser.add_argument('--output', help='Output JSON file (optional, otherwise prints to stdout)')

    args = parser.parse_args()

    try:
        result = detect_cells(
            model_path=args.model,
            image_path=args.image,
            conf_threshold=args.conf,
            iou_threshold=args.iou,
            min_cell_area=args.min_area,
            device=args.device
        )

        # Output result as JSON
        json_output = json.dumps(result, indent=2)

        if args.output:
            with open(args.output, 'w') as f:
                f.write(json_output)
        else:
            print(json_output)

        return 0

    except Exception as e:
        error_result = {
            'success': False,
            'error': str(e)
        }
        print(json.dumps(error_result))
        return 1


if __name__ == '__main__':
    sys.exit(main())
