// imageprocessor.cpp - OPTIMIZED VERSION
#include "imageprocessor.h"
#include "utils.h"
#include "logger.h"
#include <cctype>
#include <string>
#include <regex>
#include <opencv2/imgcodecs.hpp>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QImage>
#include <QFuture>

ImageProcessor::ImageProcessor() : m_enableParallel(true), m_debugMode(false) {
    LOG_INFO("ImageProcessor created");
}

ImageProcessor::~ImageProcessor() {
    LOG_INFO("ImageProcessor destroyed");
}

void ImageProcessor::processImages(const QStringList& paths, const HoughParams& params, const PreprocessingParams& prepParams) {
    cells.clear();
    m_lastError.clear();
    
    LOG_INFO(QString("Processing %1 images").arg(paths.size()));
    
    if (m_enableParallel && paths.size() > 1) {
        processImagesParallel(paths, params, prepParams);
    } else {
        processImagesSequential(paths, params, prepParams);
    }
    
    LOG_INFO(QString("Processing complete. Detected %1 cells total").arg(cells.size()));
}

void ImageProcessor::processImagesSequential(const QStringList& paths, const HoughParams& params, const PreprocessingParams& prepParams) {
    for (const QString& path : paths) {
        try {
            processSingleImage(path, params, prepParams);
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Failed to process %1: %2").arg(path).arg(e.what()));
            m_lastError = QString("Error processing %1: %2").arg(path).arg(e.what());
        }
    }
}

void ImageProcessor::processImagesParallel(const QStringList& paths, const HoughParams& params, const PreprocessingParams& prepParams) {
    QVector<QFuture<QVector<Cell>>> futures;
    
    for (const QString& path : paths) {
        futures.append(QtConcurrent::run([this, path, params, prepParams]() {
            QVector<Cell> localCells;
            try {
                localCells = processSingleImageThreadSafe(path, params, prepParams);
            } catch (const std::exception& e) {
                LOG_ERROR(QString("Parallel processing failed for %1: %2").arg(path).arg(e.what()));
            }
            return localCells;
        }));
    }
    
    // Collect results
    for (auto& future : futures) {
        QVector<Cell> localCells = future.result();
        cells.append(localCells);
    }
}

QVector<Cell> ImageProcessor::processSingleImageThreadSafe(const QString& path, const HoughParams& params, const PreprocessingParams& prepParams) {
    QVector<Cell> localCells;
    
    cv::Mat src = loadImageSafely(path);
    if (src.empty()) {
        LOG_WARNING(QString("Failed to load image: %1").arg(path));
        return localCells;
    }
    
    // Apply preprocessing if enabled
    cv::Mat processed = applyPreprocessing(src, prepParams);
    
    cv::Mat gray, blurred;
    cv::cvtColor(processed, gray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(gray, blurred, 5);
    
    std::vector<cv::Vec3f> circles;
    detectCircles(blurred, circles, params);
    
    // Detect scale for μm conversion
    double umPerPixel = detectAndCalculateScale(src);
    
    int padding = 30;
    for (const auto& c : circles) {
        int x = cvRound(c[0]);
        int y = cvRound(c[1]);
        int r = cvRound(c[2]);
        
        double visibleRatio = visibleCircleRatio(x, y, r, src.cols, src.rows);
        if (visibleRatio < 0.6) {
            continue;
        }
        
        // Create cell with padding for preview
        Cell cell = createCell(src, c, padding, path.toStdString());
        
        // Apply scale if detected
        if (umPerPixel > 0) {
            cell.diameterNm = cell.diameterPx * umPerPixel;
            cell.diameter_nm = cell.diameter_pixels * umPerPixel;
        }
        
        localCells.append(cell);
    }
    
    return localCells;
}

void ImageProcessor::processSingleImage(const QString& path, const HoughParams& params, const PreprocessingParams& prepParams) {
    cv::Mat src = loadImageSafely(path);
    if (src.empty()) {
        throw std::runtime_error("Failed to load image: " + path.toStdString());
    }

    LOG_DEBUG(QString("Processing image: %1 (%2x%3)")
        .arg(path)
        .arg(src.cols)
        .arg(src.rows));

    // Apply preprocessing if enabled
    cv::Mat processed = applyPreprocessing(src, prepParams);

    cv::Mat gray, blurred;
    cv::cvtColor(processed, gray, cv::COLOR_BGR2GRAY);
    cv::medianBlur(gray, blurred, 5);

    cv::Mat srcCopy;
    if (m_debugMode) {
        srcCopy = src.clone();
    }

    std::vector<cv::Vec3f> circles;
    detectCircles(blurred, circles, params);
    
    LOG_DEBUG(QString("Detected %1 circles").arg(circles.size()));

    // Detect scale for μm conversion
    double umPerPixel = detectAndCalculateScale(src);
    if (umPerPixel > 0) {
        LOG_INFO(QString("Scale detected: %1 μm/pixel").arg(umPerPixel));
    }

    int padding = 30;
    for (const auto& c : circles) {
        int x = cvRound(c[0]);
        int y = cvRound(c[1]);
        int r = cvRound(c[2]);

        double visibleRatio = visibleCircleRatio(x, y, r, src.cols, src.rows);
        if (visibleRatio < 0.6) {
            LOG_DEBUG(QString("Skipping circle at (%1,%2) - visibility %3%")
                .arg(x).arg(y).arg(visibleRatio * 100));
            continue;
        }

        if (m_debugMode && !srcCopy.empty()) {
            cv::Rect rectForDrawing(x - r, y - r, 2 * r, 2 * r);
            cv::rectangle(srcCopy, rectForDrawing, cv::Scalar(0, 0, 255), 2);
        }

        Cell cell = createCell(src, c, padding, path.toStdString());
        
        // Apply scale if detected
        if (umPerPixel > 0) {
            cell.diameterNm = cell.diameterPx * umPerPixel;
            cell.diameter_nm = cell.diameter_pixels * umPerPixel;
        }
        
        cells.append(cell);
    }

    if (m_debugMode && !srcCopy.empty()) {
        std::string debugPath = "debug_" + QFileInfo(path).baseName().toStdString() + ".png";
        cv::imwrite(debugPath, srcCopy);
        LOG_DEBUG(QString("Debug image saved: %1").arg(QString::fromStdString(debugPath)));
    }
}

void ImageProcessor::detectCircles(const cv::Mat& blurred, std::vector<cv::Vec3f>& circles, 
                                  const HoughParams& params) {
    try {
        cv::HoughCircles(
            blurred, circles,
            cv::HOUGH_GRADIENT,
            params.dp,
            params.minDist,
            params.param1,
            params.param2,
            params.minRadius,
            params.maxRadius
        );
    } catch (const cv::Exception& e) {
        LOG_ERROR(QString("HoughCircles failed: %1").arg(e.what()));
        throw std::runtime_error("Circle detection failed: " + std::string(e.what()));
    }
}

Cell ImageProcessor::createCell(const cv::Mat& src, const cv::Vec3f& circle, 
                               int padding, const std::string& imagePath) {
    int x = cvRound(circle[0]);
    int y = cvRound(circle[1]);
    int r = cvRound(circle[2]);
    
    // Calculate ROI with padding
    int roiX = std::max(x - r - padding, 0);
    int roiY = std::max(y - r - padding, 0);
    int roiW = std::min(2 * r + 2 * padding, src.cols - roiX);
    int roiH = std::min(2 * r + 2 * padding, src.rows - roiY);
    
    cv::Rect rectForCrop(roiX, roiY, roiW, roiH);
    
    Cell cell;
    cell.circle = circle;
    cell.image = src(rectForCrop).clone();
    cell.diameterPx = 2 * r;
    cell.imagePath = imagePath;
    
    // Set new fields for statistics
    cell.center_x = x;
    cell.center_y = y;
    cell.radius = r;
    cell.diameter_pixels = 2 * r;
    cell.area = static_cast<int>(M_PI * r * r);
    cell.cellImage = cell.image.clone();
    
    return cell;
}

double ImageProcessor::detectAndCalculateScale(const cv::Mat& image) {
    // First try to detect scale bar
    cv::Vec4i scaleLine = detectScaleLine(image);
    
    if (scaleLine[0] == 0 && scaleLine[1] == 0 && scaleLine[2] == 0 && scaleLine[3] == 0) {
        LOG_DEBUG("No scale line detected");
        return 0.0;
    }
    
    // Calculate line length
    double lineLength = calculateLineLength(scaleLine);
    if (lineLength < 50) { // Too short to be a scale bar
        return 0.0;
    }
    
    // Try to detect scale text near the line
    double scaleValue = detectScaleValue(image, scaleLine);
    
    if (scaleValue > 0) {
        double umPerPixel = (scaleValue / 1000.0) / lineLength;  // Convert nm to μm
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
    
    // Focus on bottom 25% of image where scale bars usually are
    int bottomStart = gray.rows * 0.75;
    cv::Mat bottomRegion = gray(cv::Rect(0, bottomStart, gray.cols, gray.rows - bottomStart));
    
    // Apply edge detection
    cv::Mat edges;
    cv::Canny(bottomRegion, edges, 50, 150, 3);
    
    // Detect lines using Hough transform
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(edges, lines, 1, CV_PI/180, 80, 50, 10);
    
    // Find the most horizontal line with sufficient length
    cv::Vec4i bestLine(0, 0, 0, 0);
    double maxLength = 0;
    
    for (const auto& line : lines) {
        double dx = line[2] - line[0];
        double dy = line[3] - line[1];
        double length = std::sqrt(dx*dx + dy*dy);
        double angle = std::abs(std::atan2(dy, dx) * 180 / CV_PI);
        
        // Look for horizontal lines (angle close to 0 or 180)
        if ((angle < 5 || angle > 175) && length > maxLength && length > 50) {
            maxLength = length;
            bestLine = line;
            // Adjust Y coordinate back to full image coordinates
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
    // Define region around the scale line to search for text
    int x1 = std::min(line[0], line[2]);
    int y1 = std::min(line[1], line[3]);
    int width = std::abs(line[2] - line[0]);
    
    // Look above and below the line for text
    int searchHeight = 100;
    int roiY = std::max(0, y1 - searchHeight/2);
    int roiHeight = std::min(searchHeight, image.rows - roiY);
    int roiX = std::max(0, x1 - 50);
    int roiWidth = std::min(width + 100, image.cols - roiX);
    
    if (roiWidth <= 0 || roiHeight <= 0) {
        return 0.0;
    }
    
    cv::Mat roi = image(cv::Rect(roiX, roiY, roiWidth, roiHeight));
    
    // Try to extract text using OCR or pattern matching
    // For now, use simple pattern matching for common scale values
    double detectedValue = detectScalePatterns(roi);
    
    return detectedValue;
}

double ImageProcessor::detectScalePatterns(const cv::Mat& roi) {
    if (roi.empty()) return 0.0;
    
    // Convert to grayscale if needed
    cv::Mat gray;
    if (roi.channels() == 3) {
        cv::cvtColor(roi, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = roi.clone();
    }
    
    // Apply threshold to get binary image
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    
    // Common scale values in nanometers that appear in microscopy (will be converted to μm)
    std::vector<double> commonScales = {10, 20, 25, 50, 100, 200, 250, 500, 1000, 2000, 5000, 10000};
    
    // Use morphological operations to clean up text
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    
    // Find contours to locate text regions
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Look for rectangular regions that might contain numbers
    for (const auto& contour : contours) {
        cv::Rect boundingRect = cv::boundingRect(contour);
        
        // Filter by size - text should be reasonably sized
        if (boundingRect.width < 10 || boundingRect.height < 5 || 
            boundingRect.width > roi.cols/2 || boundingRect.height > roi.rows/2) {
            continue;
        }
        
        // Extract the region
        cv::Mat textROI = binary(boundingRect);
        
        // Calculate white pixel density
        int whitePixels = cv::countNonZero(textROI);
        double density = (double)whitePixels / (textROI.rows * textROI.cols);
        
        // Text regions typically have 10-50% white pixels
        if (density > 0.1 && density < 0.5) {
            // Try to match against common scales based on region characteristics
            // Longer regions might contain larger numbers
            double aspectRatio = (double)boundingRect.width / boundingRect.height;
            
            if (aspectRatio > 2.0) { // Wide regions suggest multi-digit numbers
                for (double scale : {100, 200, 250, 500, 1000, 2000, 5000}) {
                    // Return the most likely scale based on region size
                    if (boundingRect.width > 20) return scale;
                }
            } else { // Smaller regions suggest single/double digit numbers
                for (double scale : {10, 20, 25, 50, 100}) {
                    if (boundingRect.width <= 20) return scale;
                }
            }
        }
    }
    
    // If no text detected, return most common default
    LOG_DEBUG("No scale text detected, using heuristic estimation");
    
    // Estimate based on typical microscopy usage
    // Most cell images use 100nm or 200nm scale bars (converted to μm in calculations)
    if (roi.cols > 50) {
        return 100.0; // Default for most cell microscopy
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

void ImageProcessor::setParallelProcessing(bool enable) {
    m_enableParallel = enable;
    LOG_INFO(QString("Parallel processing %1").arg(enable ? "enabled" : "disabled"));
}

void ImageProcessor::setPreprocessingParams(const PreprocessingParams& params) {
    m_preprocessingParams = params;
    LOG_INFO("Preprocessing parameters updated");
}

cv::Mat ImageProcessor::applyPreprocessing(const cv::Mat& input, const PreprocessingParams& params) {
    cv::Mat result = input.clone();
    
    try {
        // Apply contrast and brightness adjustment
        if (params.adjustContrast) {
            result.convertTo(result, -1, params.contrastAlpha, params.contrastBeta);
            LOG_DEBUG(QString("Applied contrast: alpha=%1, beta=%2").arg(params.contrastAlpha).arg(params.contrastBeta));
        }
        
        // Apply Gaussian blur
        if (params.applyGaussianBlur && params.gaussianKernel > 0) {
            int kernel = params.gaussianKernel;
            if (kernel % 2 == 0) kernel++; // Ensure odd kernel size
            cv::GaussianBlur(result, result, cv::Size(kernel, kernel), 0);
            LOG_DEBUG(QString("Applied Gaussian blur: kernel=%1").arg(kernel));
        }
        
        // Apply bilateral filter (good for noise reduction while preserving edges)
        if (params.applyBilateralFilter) {
            cv::Mat filtered;
            cv::bilateralFilter(result, filtered, params.bilateralD, 
                              params.bilateralSigmaColor, params.bilateralSigmaSpace);
            result = filtered;
            LOG_DEBUG(QString("Applied bilateral filter: d=%1, sigmaColor=%2, sigmaSpace=%3")
                .arg(params.bilateralD).arg(params.bilateralSigmaColor).arg(params.bilateralSigmaSpace));
        }
        
        // Enhance edges
        if (params.enhanceEdges && params.edgeStrength > 0) {
            cv::Mat gray, edges, enhanced;
            
            if (result.channels() == 3) {
                cv::cvtColor(result, gray, cv::COLOR_BGR2GRAY);
            } else {
                gray = result.clone();
            }
            
            // Detect edges using Canny
            cv::Canny(gray, edges, 50, 150);
            
            // Convert edges back to color if needed
            if (result.channels() == 3) {
                cv::cvtColor(edges, enhanced, cv::COLOR_GRAY2BGR);
            } else {
                enhanced = edges;
            }
            
            // Blend original with enhanced edges
            cv::addWeighted(result, 1.0, enhanced, params.edgeStrength * 0.1, 0, result);
            LOG_DEBUG(QString("Applied edge enhancement: strength=%1").arg(params.edgeStrength));
        }
        
    } catch (const cv::Exception& e) {
        LOG_ERROR(QString("Preprocessing failed: %1").arg(e.what()));
        return input; // Return original on error
    }
    
    return result;
}

cv::Mat ImageProcessor::loadImageSafely(const QString& imagePath) {
    // Попробуем загрузить стандартным способом
    cv::Mat image = cv::imread(imagePath.toStdString());
    
    if (!image.empty()) {
        return image;
    }
    
    // Если не получилось, попробуем через QImage (лучше работает с Unicode)
    QImage qImage;
    if (!qImage.load(imagePath)) {
        LOG_ERROR("Не удалось загрузить изображение: " + imagePath);
        return cv::Mat();
    }
    
    // Конвертируем QImage в cv::Mat
    QImage rgbImage = qImage.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3, (void*)rgbImage.constBits(), rgbImage.bytesPerLine());
    cv::Mat result;
    cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
    
    LOG_INFO("Изображение загружено через QImage: " + imagePath);
    return result.clone();
}

void ImageProcessor::processImagesAdvanced(const QStringList& paths, const AdvancedDetector::DetectionParams& params,
                                         const PreprocessingParams& prepParams) {
    QMutexLocker locker(&m_mutex);
    
    cells.clear();
    m_lastError.clear();
    
    if (paths.isEmpty()) {
        m_lastError = "Список путей к изображениям пуст";
        LOG_WARNING(m_lastError);
        return;
    }
    
    LOG_INFO(QString("Начало расширенной обработки %1 изображений алгоритмом %2")
             .arg(paths.size())
             .arg(AdvancedDetector::getAlgorithmDescription(params.algorithm)));
    
    try {
        for (const QString& path : paths) {
            cv::Mat image = loadImageSafely(path);
            if (image.empty()) {
                LOG_WARNING("Не удалось загрузить изображение: " + path);
                continue;
            }
            
            // Применяем предобработку
            cv::Mat processedImage = applyPreprocessing(image, prepParams);
            
            // Обнаружение клеток с помощью продвинутых алгоритмов
            QVector<Cell> detectedCells = m_advancedDetector.detectCells(processedImage, params);
            
            // Обновляем пути к изображениям для каждой клетки
            for (Cell& cell : detectedCells) {
                cell.imagePath = path.toStdString();
                
                // Вычисляем масштаб и диаметр в нанометрах
                double scale = detectAndCalculateScale(image);
                if (scale > 0) {
                    cell.diameter_nm = cell.diameter_pixels * scale;
                    cell.diameterNm = cell.diameter_nm;  // Also set compatibility field
                }
            }
            
            cells.append(detectedCells);
            
            LOG_INFO(QString("Обнаружено %1 клеток в изображении %2")
                     .arg(detectedCells.size())
                     .arg(QFileInfo(path).baseName()));
        }
        
        LOG_INFO(QString("Расширенная обработка завершена. Всего обнаружено %1 клеток").arg(cells.size()));
        
    } catch (const cv::Exception& e) {
        m_lastError = QString("Ошибка OpenCV при расширенной обработке: %1").arg(e.what());
        LOG_ERROR(m_lastError);
    } catch (const std::exception& e) {
        m_lastError = QString("Ошибка при расширенной обработке: %1").arg(e.what());
        LOG_ERROR(m_lastError);
    }
}