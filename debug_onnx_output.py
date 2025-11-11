#!/usr/bin/env python3
"""
Debug ONNX model output to understand the raw values
"""

import cv2
import numpy as np
import onnxruntime as ort


def analyze_output(output):
    """Analyze raw ONNX output"""
    print(f"Output shape: {output.shape}")
    print(f"Output dtype: {output.dtype}")

    if output.shape[2] == 8400:
        # NON-transposed: [1, features, 8400]
        num_features = output.shape[1]
        num_detections = output.shape[2]

        print(f"\nFormat: [1, {num_features}, {num_detections}] - NON-transposed")

        # Analyze first few detections
        print("\nFirst 10 detections (raw values):")
        for i in range(10):
            x = output[0, 0, i]
            y = output[0, 1, i]
            w = output[0, 2, i]
            h = output[0, 3, i]

            # Get class scores
            scores = output[0, 4:, i]
            max_score = np.max(scores)
            max_idx = np.argmax(scores)

            print(f"  Det {i}: x={x:.4f}, y={y:.4f}, w={w:.4f}, h={h:.4f}, "
                  f"max_score={max_score:.4f} (class {max_idx})")

        # Analyze score distribution
        all_scores = output[0, 4:, :].flatten()
        print(f"\nScore statistics (all {len(all_scores)} values):")
        print(f"  Min: {np.min(all_scores):.4f}")
        print(f"  Max: {np.max(all_scores):.4f}")
        print(f"  Mean: {np.mean(all_scores):.4f}")
        print(f"  Median: {np.median(all_scores):.4f}")
        print(f"  Std: {np.std(all_scores):.4f}")

        # Check if scores are logits or probabilities
        print(f"\n  Scores > 1.0: {np.sum(all_scores > 1.0)} ({100*np.sum(all_scores > 1.0)/len(all_scores):.2f}%)")
        print(f"  Scores < 0.0: {np.sum(all_scores < 0.0)} ({100*np.sum(all_scores < 0.0)/len(all_scores):.2f}%)")

        # Analyze maximum scores per detection
        max_scores_per_detection = np.max(output[0, 4:, :], axis=0)
        print(f"\nMax score per detection ({num_detections} detections):")
        print(f"  Min: {np.min(max_scores_per_detection):.4f}")
        print(f"  Max: {np.max(max_scores_per_detection):.4f}")
        print(f"  Mean: {np.mean(max_scores_per_detection):.4f}")
        print(f"  Median: {np.median(max_scores_per_detection):.4f}")

        # How many detections pass different thresholds
        print(f"\nDetections passing thresholds:")
        for thresh in [0.1, 0.25, 0.5, 0.7, 0.9]:
            count = np.sum(max_scores_per_detection > thresh)
            print(f"  > {thresh}: {count} detections ({100*count/num_detections:.2f}%)")

        # Histogram of max scores
        print(f"\nHistogram of max scores (10 bins):")
        hist, bins = np.histogram(max_scores_per_detection, bins=10)
        for i in range(len(hist)):
            print(f"  [{bins[i]:.2f}, {bins[i+1]:.2f}): {hist[i]} detections")

        return max_scores_per_detection


def main():
    model_path = "ml-data/models/yolov8s_cells_v1.0.onnx"
    image_path = "cells/ТИМ/ТИМ1.jpg"

    print("=== ONNX Output Analysis ===")
    print(f"Model: {model_path}")
    print(f"Image: {image_path}\n")

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

    # Analyze
    max_scores = analyze_output(output)

    # Try applying sigmoid to see if scores are logits
    print("\n=== Testing Sigmoid Transformation ===")
    sigmoid_scores = 1 / (1 + np.exp(-max_scores))
    print(f"After sigmoid:")
    print(f"  Min: {np.min(sigmoid_scores):.4f}")
    print(f"  Max: {np.max(sigmoid_scores):.4f}")
    print(f"  Mean: {np.mean(sigmoid_scores):.4f}")
    print(f"  Median: {np.median(sigmoid_scores):.4f}")

    print(f"\nDetections passing thresholds (after sigmoid):")
    for thresh in [0.1, 0.25, 0.5, 0.7, 0.9]:
        count = np.sum(sigmoid_scores > thresh)
        print(f"  > {thresh}: {count} detections ({100*count/8400:.2f}%)")


if __name__ == '__main__':
    main()
