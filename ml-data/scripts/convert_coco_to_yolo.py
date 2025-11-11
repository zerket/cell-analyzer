#!/usr/bin/env python3
"""
Convert COCO format annotations to YOLO segmentation format.
YOLO segmentation format: one txt file per image with normalized polygon coordinates.
Format: <class_id> <x1> <y1> <x2> <y2> ... <xn> <yn>
"""

import json
import os
import shutil
import zipfile
from pathlib import Path
from typing import Dict, List
import numpy as np
from pycocotools import mask as mask_utils
import cv2


def extract_zip(zip_path: str, extract_to: str) -> str:
    """Extract zip file and return path to annotations.json"""
    print(f"Extracting {zip_path}...")
    with zipfile.ZipFile(zip_path, 'r') as zip_ref:
        zip_ref.extractall(extract_to)

    # Find annotations.json
    for root, dirs, files in os.walk(extract_to):
        if 'instances_default.json' in files:
            return os.path.join(root, 'instances_default.json')
        elif 'annotations.json' in files:
            return os.path.join(root, 'annotations.json')

    raise FileNotFoundError("No annotations file found in zip")


def coco_to_yolo_segmentation(coco_json_path: str,
                               images_dir: str,
                               output_dir: str,
                               class_mapping: Dict[str, int],
                               split_ratio: float = 0.8):
    """
    Convert COCO annotations to YOLO segmentation format.

    Args:
        coco_json_path: Path to COCO annotations.json
        images_dir: Directory with source images
        output_dir: Output directory for YOLO dataset
        class_mapping: Dict mapping class names to class IDs (e.g., {'cell': 0, 'droplet': 1})
        split_ratio: Train/val split ratio
    """
    print(f"Loading COCO annotations from {coco_json_path}")
    with open(coco_json_path, 'r') as f:
        coco_data = json.load(f)

    # Create output directories
    output_path = Path(output_dir)
    for split in ['train', 'val']:
        (output_path / 'images' / split).mkdir(parents=True, exist_ok=True)
        (output_path / 'labels' / split).mkdir(parents=True, exist_ok=True)

    # Build image id to filename mapping
    image_info = {}
    for img in coco_data['images']:
        image_info[img['id']] = {
            'file_name': img['file_name'],
            'width': img['width'],
            'height': img['height']
        }

    # Build category name to id mapping (with Russian language support)
    category_name_to_id = {}
    russian_to_english = {
        'клетка': 'cell',
        'вода': 'droplet',
        'капля': 'droplet'
    }

    for cat in coco_data['categories']:
        cat_name = cat['name'].lower()
        # Try direct match first
        if cat_name in class_mapping:
            category_name_to_id[cat['id']] = class_mapping[cat_name]
        # Try Russian translation
        elif cat_name in russian_to_english:
            english_name = russian_to_english[cat_name]
            if english_name in class_mapping:
                category_name_to_id[cat['id']] = class_mapping[english_name]

    print(f"Found {len(image_info)} images")
    print(f"Category mapping: {category_name_to_id}")

    # Group annotations by image
    annotations_by_image = {}
    for ann in coco_data['annotations']:
        img_id = ann['image_id']
        if img_id not in annotations_by_image:
            annotations_by_image[img_id] = []
        annotations_by_image[img_id].append(ann)

    # Process each image
    image_ids = list(image_info.keys())
    np.random.seed(42)
    np.random.shuffle(image_ids)

    split_idx = int(len(image_ids) * split_ratio)
    train_ids = image_ids[:split_idx]
    val_ids = image_ids[split_idx:]

    print(f"Split: {len(train_ids)} train, {len(val_ids)} val")

    stats = {'train': {'images': 0, 'annotations': 0},
             'val': {'images': 0, 'annotations': 0}}

    for img_id in image_ids:
        info = image_info[img_id]
        img_filename = info['file_name']
        img_width = info['width']
        img_height = info['height']

        # Determine split
        split = 'train' if img_id in train_ids else 'val'

        # Copy image
        src_img_path = os.path.join(images_dir, img_filename)
        if not os.path.exists(src_img_path):
            print(f"Warning: Image not found: {src_img_path}")
            continue

        dst_img_path = output_path / 'images' / split / img_filename
        shutil.copy2(src_img_path, dst_img_path)
        stats[split]['images'] += 1

        # Create label file
        label_filename = Path(img_filename).stem + '.txt'
        label_path = output_path / 'labels' / split / label_filename

        # Get annotations for this image
        anns = annotations_by_image.get(img_id, [])

        with open(label_path, 'w') as f:
            for ann in anns:
                # Get class id
                coco_cat_id = ann['category_id']
                if coco_cat_id not in category_name_to_id:
                    continue

                yolo_class_id = category_name_to_id[coco_cat_id]

                # Get segmentation
                if 'segmentation' not in ann or not ann['segmentation']:
                    print(f"Warning: No segmentation for annotation {ann['id']}")
                    continue

                segmentation = ann['segmentation']

                # Handle different segmentation formats
                polygons = []

                # Format 1: RLE (Run-Length Encoding)
                if isinstance(segmentation, dict):
                    # Decode RLE to mask
                    if 'counts' in segmentation and 'size' in segmentation:
                        # Convert to proper RLE format for pycocotools
                        rle = {
                            'counts': segmentation['counts'],
                            'size': segmentation['size']
                        }
                        # If counts is a list, need to encode it first
                        if isinstance(rle['counts'], list):
                            # Create binary mask from uncompressed RLE
                            h, w = rle['size']
                            mask = np.zeros(h * w, dtype=np.uint8)
                            counts = rle['counts']
                            pos = 0
                            for i, count in enumerate(counts):
                                if i % 2 == 1:  # odd indices are 1s
                                    mask[pos:pos+count] = 1
                                pos += count
                            mask = mask.reshape((h, w), order='F')  # Fortran order
                        else:
                            # Already compressed RLE
                            mask = mask_utils.decode(rle)

                        # Find contours in the mask
                        contours, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

                        # Convert contours to polygons
                        for contour in contours:
                            if len(contour) >= 3:
                                polygon = contour.flatten().tolist()
                                polygons.append(polygon)

                # Format 2: Polygon list
                elif isinstance(segmentation, list):
                    polygons = segmentation

                # Process each polygon
                for seg in polygons:
                    if len(seg) < 6:  # Need at least 3 points (6 coordinates)
                        continue

                    # Normalize coordinates and clamp to [0,1] range
                    # YOLO requires coordinates within [0,1] even if circles extend beyond boundaries
                    normalized_coords = []
                    for i in range(0, len(seg), 2):
                        x = max(0.0, min(1.0, seg[i] / img_width))
                        y = max(0.0, min(1.0, seg[i + 1] / img_height))
                        normalized_coords.extend([x, y])

                    # Write to file
                    line = f"{yolo_class_id} " + " ".join(f"{c:.6f}" for c in normalized_coords)
                    f.write(line + '\n')
                    stats[split]['annotations'] += 1

    print("\nConversion complete!")
    print(f"Train: {stats['train']['images']} images, {stats['train']['annotations']} annotations")
    print(f"Val: {stats['val']['images']} images, {stats['val']['annotations']} annotations")

    return stats


def create_yaml_config(output_dir: str, class_names: List[str]):
    """Create YOLO dataset configuration file"""
    yaml_path = os.path.join(output_dir, 'dataset.yaml')

    yaml_content = f"""# Cell Detection Dataset
path: {os.path.abspath(output_dir)}
train: images/train
val: images/val

# Classes
nc: {len(class_names)}
names: {class_names}
"""

    with open(yaml_path, 'w') as f:
        f.write(yaml_content)

    print(f"Created dataset config: {yaml_path}")
    return yaml_path


def main():
    import argparse

    parser = argparse.ArgumentParser(description='Convert COCO to YOLO segmentation format')
    parser.add_argument('--coco-zip', required=True, help='Path to COCO annotations zip file')
    parser.add_argument('--images-dir', required=True, help='Directory with images')
    parser.add_argument('--output-dir', required=True, help='Output directory for YOLO dataset')
    parser.add_argument('--classes', nargs='+', default=['cell', 'droplet'],
                        help='Class names in order (default: cell droplet)')
    parser.add_argument('--split-ratio', type=float, default=0.8,
                        help='Train/val split ratio (default: 0.8)')

    args = parser.parse_args()

    # Extract zip
    temp_dir = os.path.join(args.output_dir, 'temp_coco')
    os.makedirs(temp_dir, exist_ok=True)
    coco_json_path = extract_zip(args.coco_zip, temp_dir)

    # Create class mapping
    class_mapping = {cls_name.lower(): idx for idx, cls_name in enumerate(args.classes)}
    print(f"Class mapping: {class_mapping}")

    # Convert
    stats = coco_to_yolo_segmentation(
        coco_json_path=coco_json_path,
        images_dir=args.images_dir,
        output_dir=args.output_dir,
        class_mapping=class_mapping,
        split_ratio=args.split_ratio
    )

    # Create YAML config
    create_yaml_config(args.output_dir, args.classes)

    # Cleanup temp directory
    shutil.rmtree(temp_dir)
    print("\nDone!")


if __name__ == '__main__':
    main()
