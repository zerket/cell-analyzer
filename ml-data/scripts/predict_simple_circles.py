#!/usr/bin/env python3
"""
Convert YOLOv8 predictions to COCO format with simple circles.
Each cell = circle inscribed in a square bounding box.
If bbox is rectangle -> convert to square from top-left corner.
"""

import argparse
import json
from pathlib import Path
import cv2
import numpy as np
from datetime import datetime
from ultralytics import YOLO
from tqdm import tqdm
import torch


def bbox_to_square_circle(x1, y1, x2, y2, img_width, img_height, num_points=16):
    """
    Convert bbox to square, then create circle inscribed in that square.
    Expands from bottom-right if bbox is near left/top edges.
    Expands from top-left if bbox is near right/bottom edges.

    Args:
        x1, y1: top-left corner
        x2, y2: bottom-right corner
        img_width: image width
        img_height: image height
        num_points: number of points for circle polygon

    Returns:
        polygon: list of [x, y, x, y, ...] coordinates
    """
    # Calculate bbox dimensions
    width = x2 - x1
    height = y2 - y1

    # Convert to square: use max(width, height)
    square_size = max(width, height)

    # Determine anchor point based on bbox position
    # If near left or top edge → expand from bottom-right corner
    # If near right or bottom edge → expand from top-left corner

    # Define "near edge" threshold (e.g., within 10% of image dimensions)
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
        # This makes circle extend beyond left/top edges
        square_x1 = x2 - square_size
        square_y1 = y2 - square_size
        square_x2 = x2
        square_y2 = y2
    else:
        # Bbox near right or bottom edge (or in center) → expand from top-left corner
        # This makes circle extend beyond right/bottom edges
        square_x1 = x1
        square_y1 = y1
        square_x2 = x1 + square_size
        square_y2 = y1 + square_size

    # DO NOT clamp to boundaries - allow circles to extend beyond image edges
    # This is intentional to show full cell size even when partially visible

    # Circle inscribed in square
    center_x = square_x1 + square_size / 2
    center_y = square_y1 + square_size / 2
    radius = square_size / 2

    # Generate circle points
    angles = np.linspace(0, 2 * np.pi, num_points, endpoint=False)

    polygon = []
    for angle in angles:
        x = center_x + radius * np.cos(angle)
        y = center_y + radius * np.sin(angle)
        polygon.extend([float(x), float(y)])

    return polygon, (square_x1, square_y1, square_size, square_size)


def predict_and_convert_to_simple_circles(model_path: str,
                                          images_dir: str,
                                          output_json: str,
                                          conf_threshold: float = 0.25,
                                          iou_threshold: float = 0.7,
                                          min_cell_area: int = 500,
                                          device: str = '0',
                                          filter_droplets: bool = True,
                                          num_polygon_points: int = 16):
    """
    Run YOLOv8 predictions and convert to COCO format with simple circles in square bboxes.
    Expands bbox to square from bottom-right if near left/top edges, from top-left otherwise.
    """
    print(f"=== YOLOv8 Prediction with Simple Circles (Square BBox) ===")
    print(f"Model: {model_path}")
    print(f"Images: {images_dir}")
    print(f"Circle points: {num_polygon_points}")
    print(f"Min cell area: {min_cell_area} pixels")

    # Load model
    print(f"\nLoading model...")
    model = YOLO(model_path)

    if device != 'cpu' and torch.cuda.is_available():
        print(f"GPU: {torch.cuda.get_device_name(int(device))}")
    else:
        device = 'cpu'
        print("Using CPU")

    # Get image files
    images_path = Path(images_dir)
    image_files = sorted(list(images_path.glob('*.jpg')) + list(images_path.glob('*.png')))
    print(f"Found {len(image_files)} images")

    # Initialize COCO structure
    coco_data = {
        "info": {
            "description": "YOLOv8 Cell Predictions (Simple Circles in Squares)",
            "version": "3.0",
            "year": 2025,
            "contributor": "YOLOv8s-seg",
            "date_created": datetime.now().strftime("%Y-%m-%d %H:%M:%S")
        },
        "licenses": [],
        "images": [],
        "annotations": [],
        "categories": [
            {
                "id": 1,
                "name": "cell",
                "supercategory": ""
            }
        ]
    }

    if not filter_droplets:
        coco_data["categories"].append({
            "id": 2,
            "name": "droplet",
            "supercategory": ""
        })

    annotation_id = 1
    stats = {
        'total_cells': 0,
        'total_droplets': 0,
        'rectangles_converted_to_squares': 0,
        'already_square': 0
    }

    print("\nRunning predictions and creating circles in square bboxes...")

    for img_id, img_file in enumerate(tqdm(image_files), start=1):
        # Load image
        img = cv2.imread(str(img_file))
        height, width = img.shape[:2]

        # Add image info
        coco_data["images"].append({
            "id": img_id,
            "width": width,
            "height": height,
            "file_name": img_file.name,
            "license": 0,
            "flickr_url": "",
            "coco_url": "",
            "date_captured": ""
        })

        # Run prediction
        results = model.predict(
            source=str(img_file),
            conf=conf_threshold,
            iou=iou_threshold,
            device=device,
            verbose=False,
            save=False
        )

        result = results[0]

        if result.masks is None:
            continue

        # Process each detected object
        for idx, (mask, box, cls) in enumerate(zip(result.masks.data, result.boxes.data, result.boxes.cls)):
            # Get mask area
            mask_np = mask.cpu().numpy()
            area = mask_np.sum()

            # Get class
            class_id = int(cls.cpu().numpy())

            # Determine if cell or droplet
            is_droplet = area < min_cell_area

            if is_droplet and filter_droplets:
                stats['total_droplets'] += 1
                continue

            category_id = 2 if is_droplet else 1

            # Get bounding box from YOLO
            x1, y1, x2, y2, conf, _ = box.cpu().numpy()

            # Check if bbox is square or rectangle
            bbox_width = x2 - x1
            bbox_height = y2 - y1

            if abs(bbox_width - bbox_height) < 5:  # Already approximately square
                stats['already_square'] += 1
            else:
                stats['rectangles_converted_to_squares'] += 1

            # Convert to square and create circle
            polygon, square_bbox = bbox_to_square_circle(x1, y1, x2, y2, width, height, num_polygon_points)

            square_x, square_y, square_size, _ = square_bbox

            # Create annotation
            annotation = {
                "id": annotation_id,
                "image_id": img_id,
                "category_id": category_id,
                "segmentation": [polygon],
                "area": float(area),
                "bbox": [float(square_x), float(square_y), float(square_size), float(square_size)],
                "iscrowd": 0,
                "attributes": {
                    "occluded": False,
                    "rotation": 0.0
                }
            }

            coco_data["annotations"].append(annotation)
            annotation_id += 1

            if is_droplet:
                stats['total_droplets'] += 1
            else:
                stats['total_cells'] += 1

    # Save COCO JSON
    print(f"\nSaving COCO annotations to {output_json}")
    with open(output_json, 'w') as f:
        json.dump(coco_data, f, indent=2)

    # Print statistics
    print("\n=== Conversion Complete ===")
    print(f"Total images: {len(coco_data['images'])}")
    print(f"Total cells: {stats['total_cells']}")
    print(f"Total droplets: {stats['total_droplets']}")
    print(f"Already square bboxes: {stats['already_square']}")
    print(f"Rectangles converted to squares: {stats['rectangles_converted_to_squares']}")
    print(f"Total annotations: {len(coco_data['annotations'])}")

    return coco_data


def main():
    parser = argparse.ArgumentParser(description='Convert YOLOv8 predictions to COCO with simple circles in squares')
    parser.add_argument('--model', required=True, help='Path to YOLOv8 model')
    parser.add_argument('--images', required=True, help='Directory with images')
    parser.add_argument('--output', required=True, help='Output COCO JSON file')
    parser.add_argument('--conf', type=float, default=0.25, help='Confidence threshold')
    parser.add_argument('--iou', type=float, default=0.7, help='NMS IoU threshold')
    parser.add_argument('--min-area', type=int, default=500,
                        help='Minimum cell area (pixels) to filter droplets')
    parser.add_argument('--device', default='0', help='CUDA device or cpu')
    parser.add_argument('--include-droplets', action='store_true',
                        help='Include droplet annotations')
    parser.add_argument('--polygon-points', type=int, default=16,
                        help='Number of points for circle polygon (default: 16)')

    args = parser.parse_args()

    predict_and_convert_to_simple_circles(
        model_path=args.model,
        images_dir=args.images,
        output_json=args.output,
        conf_threshold=args.conf,
        iou_threshold=args.iou,
        min_cell_area=args.min_area,
        device=args.device,
        filter_droplets=not args.include_droplets,
        num_polygon_points=args.polygon_points
    )


if __name__ == '__main__':
    main()
