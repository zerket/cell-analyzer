#!/usr/bin/env python3
"""
Test ONNX inference to verify model outputs and compare with Python YOLO API
"""

import cv2
import numpy as np
import onnxruntime as ort
from pathlib import Path
import argparse


def preprocess_image(image_path, input_size=640):
    """Preprocess image for YOLOv8 ONNX model"""
    img = cv2.imread(image_path)
    if img is None:
        raise ValueError(f"Failed to load image: {image_path}")

    original_height, original_width = img.shape[:2]
    print(f"Original image size: {original_width}x{original_height}")

    # Resize to 640x640
    resized = cv2.resize(img, (input_size, input_size))

    # Convert BGR to RGB
    rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)

    # Normalize to [0, 1] and transpose to NCHW
    blob = rgb.astype(np.float32) / 255.0
    blob = np.transpose(blob, (2, 0, 1))  # HWC to CHW
    blob = np.expand_dims(blob, axis=0)   # Add batch dimension

    print(f"Preprocessed blob shape: {blob.shape}")

    return blob, img, (original_width, original_height)


def postprocess_detections(output, original_size, conf_threshold=0.25, iou_threshold=0.7):
    """Postprocess ONNX output to get bounding boxes"""
    original_width, original_height = original_size

    # Get output shape
    print(f"Output shape: {output.shape}")

    # Determine format
    if output.shape[2] == 8400:
        # Format: [1, features, 8400] - NON-transposed
        num_features = output.shape[1]
        num_detections = output.shape[2]
        print(f"Detected NON-transposed format: [{output.shape[0]}, {num_features}, {num_detections}]")
    elif output.shape[1] == 8400:
        # Format: [1, 8400, features] - transposed
        num_detections = output.shape[1]
        num_features = output.shape[2]
        print(f"Detected transposed format: [{output.shape[0]}, {num_detections}, {num_features}]")
        output = np.transpose(output, (0, 2, 1))  # Convert to [1, features, 8400]
    else:
        raise ValueError(f"Unknown output format: {output.shape}")

    # Calculate scale factors
    scale_x = original_width / 640.0
    scale_y = original_height / 640.0
    print(f"Scale factors: scaleX={scale_x:.4f}, scaleY={scale_y:.4f}")

    boxes = []
    confidences = []

    # Parse detections
    # IMPORTANT: For YOLOv8-seg, only first 2 features after bbox are class scores
    # Indices: 0-3=bbox, 4-5=classes, 6-37=mask coefficients
    num_classes = min(2, num_features - 4)  # Only use first 2 class scores
    print(f"Number of classes (used): {num_classes}")
    print(f"Total features (bbox + classes + mask): {num_features}")

    for i in range(num_detections):
        # Get bbox coordinates (in 640x640 space)
        x_center = output[0, 0, i]
        y_center = output[0, 1, i]
        w = output[0, 2, i]
        h = output[0, 3, i]

        # Get class scores
        max_score = 0.0
        for j in range(num_classes):
            score = output[0, 4 + j, i]
            if score > max_score:
                max_score = score

        # Filter by confidence
        if max_score < conf_threshold:
            continue

        # Convert to original image coordinates
        x1 = (x_center - w / 2.0) * scale_x
        y1 = (y_center - h / 2.0) * scale_y
        x2 = (x_center + w / 2.0) * scale_x
        y2 = (y_center + h / 2.0) * scale_y

        # Clamp to image boundaries
        x1 = max(0, min(x1, original_width - 1))
        y1 = max(0, min(y1, original_height - 1))
        x2 = max(0, min(x2, original_width))
        y2 = max(0, min(y2, original_height))

        width = int(x2 - x1)
        height = int(y2 - y1)

        if width > 0 and height > 0:
            boxes.append([int(x1), int(y1), width, height])
            confidences.append(float(max_score))

    print(f"Before NMS: {len(boxes)} detections")

    # Apply NMS
    if len(boxes) > 0:
        indices = cv2.dnn.NMSBoxes(boxes, confidences, 0.0, iou_threshold)
        if len(indices) > 0:
            filtered_boxes = [boxes[i] for i in indices]
            filtered_confidences = [confidences[i] for i in indices]
        else:
            filtered_boxes = []
            filtered_confidences = []
    else:
        filtered_boxes = []
        filtered_confidences = []

    print(f"After NMS: {len(filtered_boxes)} detections")

    return filtered_boxes, filtered_confidences


def visualize_detections(image, boxes, confidences, output_path):
    """Draw bounding boxes on image"""
    img_with_boxes = image.copy()

    for box, conf in zip(boxes, confidences):
        x, y, w, h = box

        # Draw rectangle
        cv2.rectangle(img_with_boxes, (x, y), (x + w, y + h), (0, 255, 0), 2)

        # Draw confidence
        label = f"{conf:.2f}"
        cv2.putText(img_with_boxes, label, (x, y - 5),
                   cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

    cv2.imwrite(output_path, img_with_boxes)
    print(f"Visualization saved to: {output_path}")


def main():
    parser = argparse.ArgumentParser(description='Test ONNX inference')
    parser.add_argument('--model', required=True, help='Path to ONNX model')
    parser.add_argument('--image', required=True, help='Path to test image')
    parser.add_argument('--conf', type=float, default=0.25, help='Confidence threshold')
    parser.add_argument('--iou', type=float, default=0.7, help='NMS IoU threshold')
    parser.add_argument('--output', default='test_output.jpg', help='Output visualization path')

    args = parser.parse_args()

    print("=== ONNX Inference Test ===")
    print(f"Model: {args.model}")
    print(f"Image: {args.image}")
    print(f"Conf threshold: {args.conf}")
    print(f"IoU threshold: {args.iou}")
    print()

    # Load ONNX model
    print("Loading ONNX model...")
    session = ort.InferenceSession(args.model, providers=['CPUExecutionProvider'])

    # Get input/output names
    input_name = session.get_inputs()[0].name
    output_name = session.get_outputs()[0].name
    print(f"Input name: {input_name}")
    print(f"Output name: {output_name}")
    print()

    # Preprocess image
    print("Preprocessing image...")
    blob, original_img, original_size = preprocess_image(args.image)

    # Run inference
    print("\nRunning inference...")
    output = session.run([output_name], {input_name: blob})[0]

    # Postprocess
    print("\nPostprocessing...")
    boxes, confidences = postprocess_detections(output, original_size,
                                                args.conf, args.iou)

    # Visualize
    print(f"\nDetected {len(boxes)} cells")
    if len(boxes) > 0:
        visualize_detections(original_img, boxes, confidences, args.output)

        # Print some statistics
        print("\nDetection statistics:")
        print(f"  Min confidence: {min(confidences):.4f}")
        print(f"  Max confidence: {max(confidences):.4f}")
        print(f"  Mean confidence: {np.mean(confidences):.4f}")

        # Print first 5 detections
        print("\nFirst 5 detections:")
        for i, (box, conf) in enumerate(zip(boxes[:5], confidences[:5])):
            x, y, w, h = box
            print(f"  {i+1}. bbox=[{x}, {y}, {w}, {h}], conf={conf:.4f}")

    print("\n=== Test Complete ===")


if __name__ == '__main__':
    main()
