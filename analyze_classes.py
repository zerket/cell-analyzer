#!/usr/bin/env python3
"""
Analyze which classes the model predicts
"""

import cv2
import numpy as np
import onnxruntime as ort


def main():
    model_path = "ml-data/models/yolov8s_cells_v1.0.onnx"
    image_path = "cells/ТИМ/ТИМ1.jpg"

    print("=== Class Analysis ===")

    # Load model
    session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider'])

    # Load and preprocess image
    img = cv2.imread(image_path)
    resized = cv2.resize(img, (640, 640))
    rgb = cv2.cvtColor(resized, cv2.COLOR_BGR2RGB)
    blob = rgb.astype(np.float32) / 255.0
    blob = np.transpose(blob, (2, 0, 1))
    blob = np.expand_dims(blob, axis=0)

    # Run inference
    input_name = session.get_inputs()[0].name
    output_name = session.get_outputs()[0].name
    output = session.run([output_name], {input_name: blob})[0]

    print(f"Output shape: {output.shape}")

    # Get class predictions
    # Format: [1, 38, 8400]
    # First 4 are bbox, remaining 34 are classes
    num_classes = output.shape[1] - 4
    print(f"Number of classes: {num_classes}")

    # For each detection, find which class has max score
    class_predictions = np.argmax(output[0, 4:, :], axis=0)
    max_scores = np.max(output[0, 4:, :], axis=0)

    print(f"\nClass distribution (all 8400 detections):")
    unique, counts = np.unique(class_predictions, return_counts=True)
    for class_id, count in sorted(zip(unique, counts), key=lambda x: -x[1]):
        percentage = 100 * count / 8400
        print(f"  Class {class_id}: {count} detections ({percentage:.2f}%)")

    # Filter by confidence > 0.25 and check classes
    high_conf_mask = max_scores > 0.25
    high_conf_classes = class_predictions[high_conf_mask]
    high_conf_scores = max_scores[high_conf_mask]

    print(f"\nClass distribution (conf > 0.25: {np.sum(high_conf_mask)} detections):")
    unique, counts = np.unique(high_conf_classes, return_counts=True)
    for class_id, count in sorted(zip(unique, counts), key=lambda x: -x[1]):
        percentage = 100 * count / np.sum(high_conf_mask)
        mean_score = np.mean(high_conf_scores[high_conf_classes == class_id])
        print(f"  Class {class_id}: {count} detections ({percentage:.2f}%), mean score: {mean_score:.4f}")

    # Filter by confidence > 0.7
    high_conf_mask = max_scores > 0.7
    high_conf_classes = class_predictions[high_conf_mask]
    high_conf_scores = max_scores[high_conf_mask]

    print(f"\nClass distribution (conf > 0.7: {np.sum(high_conf_mask)} detections):")
    unique, counts = np.unique(high_conf_classes, return_counts=True)
    for class_id, count in sorted(zip(unique, counts), key=lambda x: -x[1]):
        percentage = 100 * count / np.sum(high_conf_mask)
        mean_score = np.mean(high_conf_scores[high_conf_classes == class_id])
        print(f"  Class {class_id}: {count} detections ({percentage:.2f}%), mean score: {mean_score:.4f}")

    # Check if class 0 is dominant (usually background or first class)
    print(f"\nIs class 0 dominant?")
    print(f"  Class 0 predictions: {np.sum(class_predictions == 0)} / 8400 ({100*np.sum(class_predictions == 0)/8400:.2f}%)")

    # Analyze scores for class 0 vs other classes
    class_0_scores = output[0, 4, :]  # Scores for class 0
    other_classes_scores = output[0, 5:, :]  # Scores for classes 1-33

    print(f"\nClass 0 score statistics:")
    print(f"  Min: {np.min(class_0_scores):.4f}")
    print(f"  Max: {np.max(class_0_scores):.4f}")
    print(f"  Mean: {np.mean(class_0_scores):.4f}")

    print(f"\nOther classes score statistics:")
    print(f"  Min: {np.min(other_classes_scores):.4f}")
    print(f"  Max: {np.max(other_classes_scores):.4f}")
    print(f"  Mean: {np.mean(other_classes_scores):.4f}")


if __name__ == '__main__':
    main()
