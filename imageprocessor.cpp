// imageprocessor.cpp - ONNX-BASED VERSION
#include "imageprocessor.h"
#include "utils.h"
#include "logger.h"
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QImage>
#include <opencv2/dnn.hpp>
#include <algorithm>

ImageProcessor::ImageProcessor() : m_debugMode(false) {
    LOG_INFO("ImageProcessor created (ONNX-based)");
}

ImageProcessor::~ImageProcessor() {
    LOG_INFO("ImageProcessor destroyed");
}

void ImageProcessor::processImages(const QStringList& paths, const YoloParams& params) {
    cells.clear();
    m_lastError.clear();

    LOG_INFO(QString("Processing %1 images with YOLO model").arg(paths.size()));
    LOG_INFO(QString("Model: %1").arg(params.modelPath));
    LOG_INFO(QString("Confidence threshold: %1").arg(params.confThreshold));

    for (const QString& path : paths) {
        try {
            processSingleImage(path, params);
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Failed to process %1: %2").arg(path).arg(e.what()));
            m_lastError = QString("Error processing %1: %2").arg(path).arg(e.what());
        }
    }

    LOG_INFO(QString("Processing complete. Detected %1 cells total").arg(cells.size()));
}

void ImageProcessor::processSingleImage(const QString& path, const YoloParams& params) {
    LOG_DEBUG(QString("Processing image: %1").arg(path));

    // Load image to get scale
    cv::Mat src = loadImageSafely(path);
    if (src.empty()) {
        throw std::runtime_error("Failed to load image: " + path.toStdString());
    }

    // Detect cells with ONNX
    QVector<Cell> detectedCells = detectCellsWithONNX(path, params);

    LOG_DEBUG(QString("Detected %1 cells").arg(detectedCells.size()));

    // Detect scale for μm conversion
    double umPerPixel = detectAndCalculateScale(src);
    if (umPerPixel > 0) {
        LOG_INFO(QString("Scale detected: %1 μm/pixel").arg(umPerPixel));
    }

    // Apply scale and create cell images
    for (Cell& cell : detectedCells) {
        // Apply scale if detected
        if (umPerPixel > 0) {
            cell.diameterNm = cell.diameterPx * umPerPixel;
            cell.diameter_um = cell.diameter_pixels * umPerPixel;
        }

        // Create cell image (crop from original with padding)
        int padding = 30;
        int x = cell.center_x;
        int y = cell.center_y;
        int r = cell.radius;

        // Calculate ROI with padding
        int roiX = std::max(x - r - padding, 0);
        int roiY = std::max(y - r - padding, 0);
        int roiW = std::min(2 * r + 2 * padding, src.cols - roiX);
        int roiH = std::min(2 * r + 2 * padding, src.rows - roiY);

        if (roiW > 0 && roiH > 0) {
            cv::Rect rectForCrop(roiX, roiY, roiW, roiH);
            cell.image = src(rectForCrop).clone();
            cell.cellImage = cell.image.clone();
        }

        cells.append(cell);
    }

    if (m_debugMode) {
        cv::Mat srcCopy = src.clone();
        for (const Cell& cell : detectedCells) {
            // Draw circle on debug image
            cv::circle(srcCopy, cv::Point(cell.center_x, cell.center_y), cell.radius,
                      cv::Scalar(0, 255, 0), 2);
        }
        std::string debugPath = "debug_" + QFileInfo(path).baseName().toStdString() + ".png";
        cv::imwrite(debugPath, srcCopy);
        LOG_DEBUG(QString("Debug image saved: %1").arg(QString::fromStdString(debugPath)));
    }
}

QVector<Cell> ImageProcessor::detectCellsWithONNX(const QString& imagePath, const YoloParams& params) {
    QVector<Cell> detectedCells;

    // Find model path - try multiple locations
    QString appDir = QCoreApplication::applicationDirPath();
    QString modelPath;

    QStringList modelSearchPaths = {
        QDir(appDir).filePath(params.modelPath),
        QDir(appDir).filePath("../" + params.modelPath),
        QDir(appDir).filePath("../../" + params.modelPath),
        params.modelPath,
        QString("cell-analyzer/%1").arg(params.modelPath)
    };

    for (const QString& path : modelSearchPaths) {
        if (QFile::exists(path)) {
            modelPath = QFileInfo(path).absoluteFilePath();
            LOG_DEBUG(QString("Found ONNX model at: %1").arg(modelPath));
            break;
        }
    }

    if (modelPath.isEmpty() || !QFile::exists(modelPath)) {
        LOG_ERROR(QString("ONNX model not found. Searched in:"));
        for (const QString& path : modelSearchPaths) {
            LOG_ERROR(QString("  - %1").arg(path));
        }
        throw std::runtime_error("ONNX model not found");
    }

    LOG_INFO(QString("Loading ONNX model: %1").arg(modelPath));

    // Load ONNX model
    cv::dnn::Net net;
    try {
        net = cv::dnn::readNetFromONNX(modelPath.toStdString());
    } catch (const cv::Exception& e) {
        LOG_ERROR(QString("Failed to load ONNX model: %1").arg(e.what()));
        throw std::runtime_error("Failed to load ONNX model: " + std::string(e.what()));
    }

    // Set backend and target
    if (params.useCUDA) {
        LOG_INFO("Using CUDA backend");
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
    } else {
        LOG_INFO("Using CPU backend");
        net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
    }

    // Load and preprocess image
    cv::Mat srcImage = loadImageSafely(imagePath);
    if (srcImage.empty()) {
        throw std::runtime_error("Failed to load image for ONNX inference");
    }

    LOG_DEBUG(QString("Image size: %1x%2").arg(srcImage.cols).arg(srcImage.rows));

    // Preprocess image for YOLOv8 (640x640, normalized)
    cv::Mat blob = preprocessImage(srcImage);

    // Run inference
    net.setInput(blob);
    std::vector<cv::Mat> outputs;
    net.forward(outputs, net.getUnconnectedOutLayersNames());

    LOG_DEBUG(QString("ONNX inference completed, outputs: %1").arg(outputs.size()));

    if (outputs.empty()) {
        LOG_ERROR("No outputs from ONNX model");
        return detectedCells;
    }

    // Postprocess results
    detectedCells = postprocessONNX(outputs[0], srcImage, imagePath, params);

    LOG_INFO(QString("ONNX detected %1 cells in %2").arg(detectedCells.size()).arg(imagePath));

    return detectedCells;
}

cv::Mat ImageProcessor::preprocessImage(const cv::Mat& image) {
    // YOLOv8 expects 640x640 input
    int inputWidth = 640;
    int inputHeight = 640;

    cv::Mat resized;
    cv::resize(image, resized, cv::Size(inputWidth, inputHeight));

    // Convert BGR to RGB and normalize to [0, 1]
    cv::Mat rgb;
    cv::cvtColor(resized, rgb, cv::COLOR_BGR2RGB);

    // Create blob: convert to float, normalize, and transpose to NCHW format
    cv::Mat blob = cv::dnn::blobFromImage(rgb, 1.0 / 255.0, cv::Size(inputWidth, inputHeight),
                                          cv::Scalar(0, 0, 0), true, false);

    return blob;
}

QVector<Cell> ImageProcessor::postprocessONNX(const cv::Mat& output, const cv::Mat& originalImage,
                                               const QString& imagePath, const YoloParams& params) {
    QVector<Cell> detectedCells;

    // Check output dimensions
    if (output.dims != 3) {
        LOG_ERROR(QString("Invalid output dimensions: %1").arg(output.dims));
        return detectedCells;
    }

    int batchSize = output.size[0];
    int dim1 = output.size[1];
    int dim2 = output.size[2];

    LOG_INFO(QString("Output shape: [%1, %2, %3]").arg(batchSize).arg(dim1).arg(dim2));
    LOG_INFO(QString("Image size: %1x%2").arg(originalImage.cols).arg(originalImage.rows));

    // Determine output format
    // YOLOv8 can be: [1, 84, 8400] or [1, 8400, 84] or [1, 38, 8400] for custom models
    int numFeatures, numDetections;

    if (dim2 == 8400) {
        // Format: [1, features, 8400] - NON-transposed (old format)
        numFeatures = dim1;
        numDetections = dim2;
        LOG_INFO(QString("Detected NON-transposed format: [1, %1, %2]").arg(numFeatures).arg(numDetections));
    } else if (dim1 == 8400) {
        // Format: [1, 8400, features] - transposed (new format)
        numDetections = dim1;
        numFeatures = dim2;
        LOG_INFO(QString("Detected transposed format: [1, %1, %2]").arg(numDetections).arg(numFeatures));
    } else {
        LOG_ERROR(QString("Unknown output format: [%1, %2, %3]").arg(batchSize).arg(dim1).arg(dim2));
        return detectedCells;
    }

    // Calculate scale factors from 640x640 to original image size
    float scaleX = (float)originalImage.cols / 640.0f;
    float scaleY = (float)originalImage.rows / 640.0f;

    LOG_INFO(QString("Scale factors: scaleX=%1, scaleY=%2").arg(scaleX).arg(scaleY));

    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;

    // Parse detections based on format
    int loggedCount = 0;
    if (dim2 == 8400) {
        // NON-transposed: [1, features, 8400]
        // Need to access as [0, feature_idx, detection_idx]
        // For YOLOv8-seg: indices 0-3 = bbox, 4-5 = class scores (2 classes), 6-37 = mask coefficients (32)
        for (int i = 0; i < numDetections; i++) {
            // Get bbox coordinates
            float xCenter = output.at<float>(0, 0, i);
            float yCenter = output.at<float>(0, 1, i);
            float w = output.at<float>(0, 2, i);
            float h = output.at<float>(0, 3, i);

            // Get class score(s)
            // IMPORTANT: For YOLOv8-seg with 2 classes, indices 4-5 are class scores, rest are mask coefficients
            // We only check the first 2 class scores (indices 4-5)
            float maxScore = 0.0f;
            int numClasses = std::min(2, numFeatures - 4);  // Only use first 2 scores for classes

            for (int j = 0; j < numClasses; j++) {
                float score = output.at<float>(0, 4 + j, i);
                if (score > maxScore) {
                    maxScore = score;
                }
            }

            // Log first few detections for debugging
            if (loggedCount < 5 && maxScore > 0.01) {
                LOG_INFO(QString("Detection %1: x=%2, y=%3, w=%4, h=%5, score=%6")
                    .arg(i).arg(xCenter).arg(yCenter).arg(w).arg(h).arg(maxScore));
                loggedCount++;
            }

            // Filter by confidence threshold
            if (maxScore < params.confThreshold) {
                continue;
            }

            // Convert from center format to corner format and scale to original image
            float x1 = (xCenter - w / 2.0f) * scaleX;
            float y1 = (yCenter - h / 2.0f) * scaleY;
            float x2 = (xCenter + w / 2.0f) * scaleX;
            float y2 = (yCenter + h / 2.0f) * scaleY;

            // Clamp to image boundaries
            x1 = std::max(0.0f, std::min(x1, (float)originalImage.cols - 1));
            y1 = std::max(0.0f, std::min(y1, (float)originalImage.rows - 1));
            x2 = std::max(0.0f, std::min(x2, (float)originalImage.cols));
            y2 = std::max(0.0f, std::min(y2, (float)originalImage.rows));

            int width = static_cast<int>(x2 - x1);
            int height = static_cast<int>(y2 - y1);

            if (width > 0 && height > 0) {
                boxes.push_back(cv::Rect(static_cast<int>(x1), static_cast<int>(y1), width, height));
                confidences.push_back(maxScore);
            }
        }
    } else {
        // Transposed: [1, 8400, features]
        // For YOLOv8-seg: indices 0-3 = bbox, 4-5 = class scores (2 classes), 6-37 = mask coefficients (32)
        for (int i = 0; i < numDetections; i++) {
            const float* detection = output.ptr<float>(0, i);

            float xCenter = detection[0];
            float yCenter = detection[1];
            float w = detection[2];
            float h = detection[3];

            float maxScore = 0.0f;
            int numClasses = std::min(2, numFeatures - 4);  // Only use first 2 scores for classes

            for (int j = 0; j < numClasses; j++) {
                float score = detection[4 + j];
                if (score > maxScore) {
                    maxScore = score;
                }
            }

            if (maxScore < params.confThreshold) {
                continue;
            }

            float x1 = (xCenter - w / 2.0f) * scaleX;
            float y1 = (yCenter - h / 2.0f) * scaleY;
            float x2 = (xCenter + w / 2.0f) * scaleX;
            float y2 = (yCenter + h / 2.0f) * scaleY;

            x1 = std::max(0.0f, std::min(x1, (float)originalImage.cols - 1));
            y1 = std::max(0.0f, std::min(y1, (float)originalImage.rows - 1));
            x2 = std::max(0.0f, std::min(x2, (float)originalImage.cols));
            y2 = std::max(0.0f, std::min(y2, (float)originalImage.rows));

            int width = static_cast<int>(x2 - x1);
            int height = static_cast<int>(y2 - y1);

            if (width > 0 && height > 0) {
                boxes.push_back(cv::Rect(static_cast<int>(x1), static_cast<int>(y1), width, height));
                confidences.push_back(maxScore);
            }
        }
    }

    LOG_INFO(QString("Before NMS: %1 detections").arg(boxes.size()));

    // Apply Non-Maximum Suppression
    // Note: We already filtered by confThreshold above, so use 0.0 here to avoid double filtering
    std::vector<int> indices;
    cv::dnn::NMSBoxes(boxes, confidences, 0.0f, params.iouThreshold, indices);

    LOG_INFO(QString("After NMS: %1 detections").arg(indices.size()));

    // Create Cell objects
    for (int idx : indices) {
        cv::Rect box = boxes[idx];

        // Calculate area
        int area = box.width * box.height;
        if (area < params.minCellArea) {
            continue;
        }

        // Convert bbox to circle (inscribed circle)
        int diameter = std::max(box.width, box.height);
        int radius = diameter / 2;
        int centerX = box.x + box.width / 2;
        int centerY = box.y + box.height / 2;

        // Clamp to image boundaries
        centerX = std::max(0, std::min(centerX, originalImage.cols - 1));
        centerY = std::max(0, std::min(centerY, originalImage.rows - 1));

        Cell cell;
        cell.center_x = centerX;
        cell.center_y = centerY;
        cell.radius = radius;
        cell.diameter_pixels = diameter;
        cell.diameterPx = diameter;
        cell.area = area;
        cell.confidence = confidences[idx];
        cell.imagePath = imagePath.toStdString();
        cell.circle = cv::Vec3f(centerX, centerY, radius);
        cell.diameter_um = 0.0;
        cell.diameterNm = 0.0;

        detectedCells.append(cell);
    }

    return detectedCells;
}

cv::Mat ImageProcessor::loadImageSafely(const QString& imagePath) {
    // Check if path contains non-ASCII characters (Cyrillic, etc.)
    bool hasUnicode = false;
    for (QChar c : imagePath) {
        if (c.unicode() > 127) {
            hasUnicode = true;
            break;
        }
    }

    // For Unicode paths, use QImage directly to avoid OpenCV warnings
    if (hasUnicode) {
        QImage qImage;
        if (!qImage.load(imagePath)) {
            LOG_ERROR("Failed to load image: " + imagePath);
            return cv::Mat();
        }

        // Convert QImage to cv::Mat
        QImage rgbImage = qImage.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3,
                   (void*)rgbImage.constBits(), rgbImage.bytesPerLine());
        cv::Mat result;
        cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);

        LOG_DEBUG("Image loaded through QImage (Unicode path): " + imagePath);
        return result.clone();
    }

    // For ASCII paths, try OpenCV directly (faster)
    cv::Mat image = cv::imread(imagePath.toStdString());

    if (!image.empty()) {
        return image;
    }

    // Fallback to QImage if OpenCV failed
    QImage qImage;
    if (!qImage.load(imagePath)) {
        LOG_ERROR("Failed to load image: " + imagePath);
        return cv::Mat();
    }

    QImage rgbImage = qImage.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3,
               (void*)rgbImage.constBits(), rgbImage.bytesPerLine());
    cv::Mat result;
    cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);

    LOG_INFO("Image loaded through QImage (fallback): " + imagePath);
    return result.clone();
}

// Scale detection methods (keep from original implementation)
double ImageProcessor::detectAndCalculateScale(const cv::Mat& image) {
    cv::Vec4i scaleLine = detectScaleLine(image);

    if (scaleLine[0] == 0 && scaleLine[1] == 0 && scaleLine[2] == 0 && scaleLine[3] == 0) {
        LOG_DEBUG("No scale line detected");
        return 0.0;
    }

    double lineLength = calculateLineLength(scaleLine);
    if (lineLength < 50) {
        return 0.0;
    }

    double scaleValue = detectScaleValue(image, scaleLine);

    if (scaleValue > 0) {
        double umPerPixel = (scaleValue / 1000.0) / lineLength;
        LOG_INFO(QString("Scale detected: %1 nm over %2 pixels = %3 μm/pixel")
            .arg(scaleValue).arg(lineLength).arg(umPerPixel));
        return umPerPixel;
    }

    return 0.0;
}

cv::Vec4i ImageProcessor::detectScaleLine(const cv::Mat& image) {
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }

    int bottomStart = gray.rows * 0.75;
    cv::Mat bottomRegion = gray(cv::Rect(0, bottomStart, gray.cols, gray.rows - bottomStart));

    cv::Mat edges;
    cv::Canny(bottomRegion, edges, 50, 150, 3);

    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(edges, lines, 1, CV_PI/180, 80, 50, 10);

    cv::Vec4i bestLine(0, 0, 0, 0);
    double maxLength = 0;

    for (const auto& line : lines) {
        double dx = line[2] - line[0];
        double dy = line[3] - line[1];
        double length = std::sqrt(dx*dx + dy*dy);
        double angle = std::abs(std::atan2(dy, dx) * 180 / CV_PI);

        if ((angle < 5 || angle > 175) && length > maxLength && length > 50) {
            maxLength = length;
            bestLine = line;
            bestLine[1] += bottomStart;
            bestLine[3] += bottomStart;
        }
    }

    return bestLine;
}

double ImageProcessor::calculateLineLength(const cv::Vec4i& line) {
    double dx = line[2] - line[0];
    double dy = line[3] - line[1];
    return std::sqrt(dx*dx + dy*dy);
}

double ImageProcessor::detectScaleValue(const cv::Mat& image, const cv::Vec4i& line) {
    int x1 = std::min(line[0], line[2]);
    int y1 = std::min(line[1], line[3]);
    int width = std::abs(line[2] - line[0]);

    int searchHeight = 100;
    int roiY = std::max(0, y1 - searchHeight/2);
    int roiHeight = std::min(searchHeight, image.rows - roiY);
    int roiX = std::max(0, x1 - 50);
    int roiWidth = std::min(width + 100, image.cols - roiX);

    if (roiWidth <= 0 || roiHeight <= 0) {
        return 0.0;
    }

    cv::Mat roi = image(cv::Rect(roiX, roiY, roiWidth, roiHeight));
    double detectedValue = detectScalePatterns(roi);

    return detectedValue;
}

double ImageProcessor::detectScalePatterns(const cv::Mat& roi) {
    if (roi.empty()) return 0.0;

    cv::Mat gray;
    if (roi.channels() == 3) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi.clone();
    }

    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    std::vector<double> commonScales = {10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000, 10000};

    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);

    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        cv::Rect boundingRect = cv::boundingRect(contour);

        if (boundingRect.width < 10 || boundingRect.height < 5 ||
            boundingRect.width > roi.cols/2 || boundingRect.height > roi.rows/2) {
            continue;
        }

        cv::Mat textROI = binary(boundingRect);
        int whitePixels = cv::countNonZero(textROI);
        double density = (double)whitePixels / (textROI.rows * textROI.cols);

        if (density > 0.1 && density < 0.5) {
            double aspectRatio = (double)boundingRect.width / boundingRect.height;

            if (aspectRatio > 2.0) {
                for (double scale : {100, 200, 250, 500, 1000, 2000, 5000}) {
                    if (boundingRect.width > 20) return scale;
                }
            } else {
                for (double scale : {10, 20, 25, 50, 100}) {
                    if (boundingRect.width <= 20) return scale;
                }
            }
        }
    }

    LOG_DEBUG("No scale text detected, using heuristic estimation");

    if (roi.cols > 50) {
        return 100.0;
    }

    return 0.0;
}

QString ImageProcessor::getLastError() const {
    return m_lastError;
}

void ImageProcessor::setDebugMode(bool enable) {
    m_debugMode = enable;
    LOG_INFO(QString("Debug mode %1").arg(enable ? "enabled" : "disabled"));
}
