// imageprocessor.cpp - YOLO-BASED VERSION
#include "imageprocessor.h"
#include "utils.h"
#include "logger.h"
#include <QProcess>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QImage>

ImageProcessor::ImageProcessor() : m_debugMode(false) {
    LOG_INFO("ImageProcessor created (YOLO-based)");
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

    // Detect cells with YOLO
    QVector<Cell> detectedCells = detectCellsWithYolo(path, params);

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

QVector<Cell> ImageProcessor::detectCellsWithYolo(const QString& imagePath, const YoloParams& params) {
    QVector<Cell> detectedCells;

    // Find Python script path - try multiple locations
    QString appDir = QCoreApplication::applicationDirPath();
    QString scriptPath;

    QStringList scriptSearchPaths = {
        QDir(appDir).filePath("ml-data/scripts/detect_cells.py"),
        QDir(appDir).filePath("../ml-data/scripts/detect_cells.py"),
        QDir(appDir).filePath("../../ml-data/scripts/detect_cells.py"),
        "ml-data/scripts/detect_cells.py",
        "cell-analyzer/ml-data/scripts/detect_cells.py"
    };

    for (const QString& path : scriptSearchPaths) {
        if (QFile::exists(path)) {
            scriptPath = QFileInfo(path).absoluteFilePath();
            LOG_DEBUG(QString("Found detection script at: %1").arg(scriptPath));
            break;
        }
    }

    if (scriptPath.isEmpty() || !QFile::exists(scriptPath)) {
        LOG_ERROR(QString("Detection script not found. Searched in:"));
        for (const QString& path : scriptSearchPaths) {
            LOG_ERROR(QString("  - %1").arg(path));
        }
        throw std::runtime_error("YOLO detection script not found");
    }

    // Find model path - try multiple locations
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
            LOG_DEBUG(QString("Found YOLO model at: %1").arg(modelPath));
            break;
        }
    }

    if (modelPath.isEmpty() || !QFile::exists(modelPath)) {
        LOG_ERROR(QString("YOLO model not found. Searched in:"));
        for (const QString& path : modelSearchPaths) {
            LOG_ERROR(QString("  - %1").arg(path));
        }
        throw std::runtime_error("YOLO model not found");
    }

    // Build command
    QStringList arguments;
    arguments << scriptPath
              << "--model" << modelPath
              << "--image" << imagePath
              << "--conf" << QString::number(params.confThreshold)
              << "--iou" << QString::number(params.iouThreshold)
              << "--min-area" << QString::number(params.minCellArea)
              << "--device" << params.device;

    // Try different Python commands (python3, python, py)
    QStringList pythonCommands = {"python", "python3", "py"};
    QString pythonCmd;
    QProcess testProcess;

    for (const QString& cmd : pythonCommands) {
        testProcess.start(cmd, QStringList() << "--version");
        if (testProcess.waitForFinished(3000) && testProcess.exitCode() == 0) {
            pythonCmd = cmd;
            LOG_DEBUG(QString("Found Python command: %1").arg(cmd));
            break;
        }
    }

    if (pythonCmd.isEmpty()) {
        LOG_ERROR("Python not found. Tried: python, python3, py");
        throw std::runtime_error("Python not found in system PATH");
    }

    LOG_INFO(QString("Running: %1 %2").arg(pythonCmd).arg(arguments.join(" ")));

    // Execute Python script
    QProcess process;
    process.start(pythonCmd, arguments);

    if (!process.waitForFinished(30000)) { // 30 second timeout
        LOG_ERROR("YOLO detection process timeout");
        throw std::runtime_error("YOLO detection timeout");
    }

    if (process.exitCode() != 0) {
        QString error = process.readAllStandardError();
        LOG_ERROR(QString("YOLO detection failed: %1").arg(error));
        throw std::runtime_error("YOLO detection failed: " + error.toStdString());
    }

    // Parse JSON output
    QByteArray output = process.readAllStandardOutput();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(output);

    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        LOG_ERROR("Failed to parse YOLO detection output");
        throw std::runtime_error("Invalid YOLO detection output");
    }

    QJsonObject result = jsonDoc.object();

    if (!result["success"].toBool()) {
        QString error = result["error"].toString();
        LOG_ERROR(QString("YOLO detection error: %1").arg(error));
        throw std::runtime_error("YOLO detection error: " + error.toStdString());
    }

    // Load source image for creating cell objects
    cv::Mat srcImage = loadImageSafely(imagePath);
    if (srcImage.empty()) {
        throw std::runtime_error("Failed to load image for cell creation");
    }

    // Parse detected cells
    QJsonArray cellsArray = result["cells"].toArray();
    LOG_INFO(QString("YOLO detected %1 cells in %2").arg(cellsArray.size()).arg(imagePath));

    for (const QJsonValue& cellValue : cellsArray) {
        QJsonObject cellData = cellValue.toObject();
        Cell cell = createCellFromYoloDetection(srcImage, cellData, imagePath.toStdString());
        detectedCells.append(cell);
    }

    return detectedCells;
}

Cell ImageProcessor::createCellFromYoloDetection(const cv::Mat& srcImage,
                                                 const QJsonObject& cellData,
                                                 const std::string& imagePath) {
    Cell cell;

    // Extract data from JSON
    cell.center_x = cellData["center_x"].toDouble();
    cell.center_y = cellData["center_y"].toDouble();
    cell.radius = cellData["radius"].toDouble();
    cell.diameter_pixels = cellData["diameter_pixels"].toDouble();
    cell.diameterPx = cell.diameter_pixels;
    cell.area = cellData["area"].toInt();
    cell.confidence = cellData["confidence"].toDouble();
    cell.imagePath = imagePath;

    // Create cv::Vec3f for compatibility (center_x, center_y, radius)
    cell.circle = cv::Vec3f(cell.center_x, cell.center_y, cell.radius);

    // Initialize diameter_um (will be filled by scale detection)
    cell.diameter_um = 0.0;
    cell.diameterNm = 0.0;

    return cell;
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
