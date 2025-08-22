#include "advanceddetector.h"
#include "logger.h"
#include <algorithm>
#include <cmath>

AdvancedDetector::AdvancedDetector() {
}

AdvancedDetector::~AdvancedDetector() {
}

QVector<Cell> AdvancedDetector::detectCells(const cv::Mat& image, const DetectionParams& params) {
    if (image.empty()) {
        Logger::instance().log("AdvancedDetector: Пустое изображение", LogLevel::WARNING);
        return QVector<Cell>();
    }

    QVector<Cell> detectedCells;
    
    try {
        switch (params.algorithm) {
            case DetectionAlgorithm::ContourBased:
                detectedCells = detectWithContours(image, params);
                break;
            case DetectionAlgorithm::WatershedSegmentation:
                detectedCells = detectWithWatershed(image, params);
                break;
            case DetectionAlgorithm::MorphologicalOperations:
                detectedCells = detectWithMorphology(image, params);
                break;
            case DetectionAlgorithm::AdaptiveThreshold:
                detectedCells = detectWithAdaptiveThreshold(image, params);
                break;
            case DetectionAlgorithm::BlobDetection:
                detectedCells = detectWithBlobDetector(image, params);
                break;
            default:
                Logger::instance().log("AdvancedDetector: Неподдерживаемый алгоритм", LogLevel::WARNING);
                return QVector<Cell>();
        }
        
        // Применяем фильтрацию по параметрам
        detectedCells = filterCellsByParams(detectedCells, params);
        
        // Удаляем перекрывающиеся клетки
        removeOverlappingCells(detectedCells);
        
        Logger::instance().log(QString("AdvancedDetector: Обнаружено %1 клеток алгоритмом %2")
                              .arg(detectedCells.size())
                              .arg(getAlgorithmDescription(params.algorithm)));
                              
    } catch (const cv::Exception& e) {
        Logger::instance().log(QString("AdvancedDetector: Ошибка OpenCV: %1").arg(e.what()), LogLevel::ERROR);
    } catch (const std::exception& e) {
        Logger::instance().log(QString("AdvancedDetector: Ошибка: %1").arg(e.what()), LogLevel::ERROR);
    }
    
    return detectedCells;
}

QVector<Cell> AdvancedDetector::detectWithContours(const cv::Mat& image, const DetectionParams& params) {
    QVector<Cell> cells;
    
    // Предобработка изображения
    cv::Mat processed = preprocessImage(image, DetectionAlgorithm::ContourBased);
    
    // Применяем адаптивное пороговое значение
    cv::Mat binary;
    cv::adaptiveThreshold(processed, binary, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, 
                         cv::THRESH_BINARY_INV, 11, 2);
    
    // Морфологические операции для очистки
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    
    // Поиск контуров
    std::vector<std::vector<cv::Point>> contours = findAndFilterContours(binary, params);
    
    // Создаем Cell объекты
    for (const auto& contour : contours) {
        Cell cell = createCellFromContour(image, contour, "");
        if (cell.area >= params.minCellArea && cell.area <= params.maxCellArea) {
            cells.append(cell);
        }
    }
    
    return cells;
}

QVector<Cell> AdvancedDetector::detectWithWatershed(const cv::Mat& image, const DetectionParams& params) {
    QVector<Cell> cells;
    
    cv::Mat processed = preprocessImage(image, DetectionAlgorithm::WatershedSegmentation);
    
    // Создаем маркеры для водораздельного алгоритма
    cv::Mat markers = createWatershedMarkers(processed, params);
    
    // Применяем водораздельный алгоритм
    cv::Mat imageColor;
    if (processed.channels() == 1) {
        cv::cvtColor(processed, imageColor, cv::COLOR_GRAY2BGR);
    } else {
        imageColor = processed.clone();
    }
    
    cv::watershed(imageColor, markers);
    
    // Находим уникальные метки (исключая границы -1 и фон 0)
    double minVal, maxVal;
    cv::minMaxLoc(markers, &minVal, &maxVal);
    
    for (int label = 2; label <= static_cast<int>(maxVal); label++) {
        cv::Mat mask = (markers == label);
        
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        if (!contours.empty()) {
            Cell cell = createCellFromContour(image, contours[0], "");
            if (cell.area >= params.minCellArea && cell.area <= params.maxCellArea) {
                cells.append(cell);
            }
        }
    }
    
    return cells;
}

QVector<Cell> AdvancedDetector::detectWithMorphology(const cv::Mat& image, const DetectionParams& params) {
    QVector<Cell> cells;
    
    cv::Mat processed = preprocessImage(image, DetectionAlgorithm::MorphologicalOperations);
    
    // Бинаризация
    cv::Mat binary;
    cv::threshold(processed, binary, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);
    
    // Морфологические операции
    cv::Mat kernel = cv::getStructuringElement(params.morphShape, 
                                              cv::Size(params.morphKernelSize, params.morphKernelSize));
    
    cv::Mat opened;
    cv::morphologyEx(binary, opened, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), params.morphIterations);
    
    // Дистанционное преобразование
    cv::Mat distTransform;
    cv::distanceTransform(opened, distTransform, cv::DIST_L2, 5);
    
    // Поиск локальных максимумов
    cv::Mat localMaxima;
    double maxVal;
    cv::minMaxLoc(distTransform, nullptr, &maxVal);
    cv::threshold(distTransform, localMaxima, 0.4 * maxVal, 255, cv::THRESH_BINARY);
    localMaxima.convertTo(localMaxima, CV_8U);
    
    // Поиск контуров на основе локальных максимумов
    std::vector<std::vector<cv::Point>> contours = findAndFilterContours(localMaxima, params);
    
    for (const auto& contour : contours) {
        Cell cell = createCellFromContour(image, contour, "");
        if (cell.area >= params.minCellArea && cell.area <= params.maxCellArea) {
            cells.append(cell);
        }
    }
    
    return cells;
}

QVector<Cell> AdvancedDetector::detectWithAdaptiveThreshold(const cv::Mat& image, const DetectionParams& params) {
    QVector<Cell> cells;
    
    cv::Mat processed = preprocessImage(image, DetectionAlgorithm::AdaptiveThreshold);
    
    // Адаптивное пороговое значение
    cv::Mat binary;
    cv::adaptiveThreshold(processed, binary, 255, params.adaptiveMethod, 
                         cv::THRESH_BINARY_INV, params.adaptiveBlockSize, params.adaptiveC);
    
    // Морфологические операции для очистки
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel);
    cv::morphologyEx(binary, binary, cv::MORPH_CLOSE, kernel);
    
    std::vector<std::vector<cv::Point>> contours = findAndFilterContours(binary, params);
    
    for (const auto& contour : contours) {
        Cell cell = createCellFromContour(image, contour, "");
        if (cell.area >= params.minCellArea && cell.area <= params.maxCellArea) {
            cells.append(cell);
        }
    }
    
    return cells;
}

QVector<Cell> AdvancedDetector::detectWithBlobDetector(const cv::Mat& image, const DetectionParams& params) {
    QVector<Cell> cells;
    
    cv::Mat processed = preprocessImage(image, DetectionAlgorithm::BlobDetection);
    
    // Создаем детектор блобов
    cv::Ptr<cv::SimpleBlobDetector> detector = createBlobDetector(params);
    
    // Обнаружение ключевых точек
    std::vector<cv::KeyPoint> keypoints;
    detector->detect(processed, keypoints);
    
    // Преобразуем ключевые точки в Cell объекты
    for (const auto& kp : keypoints) {
        Cell cell;
        cell.center_x = static_cast<int>(kp.pt.x);
        cell.center_y = static_cast<int>(kp.pt.y);
        cell.radius = static_cast<int>(kp.size / 2);
        cell.diameter_pixels = static_cast<int>(kp.size);
        cell.area = static_cast<int>(M_PI * cell.radius * cell.radius);
        
        // Создаем изображение клетки
        int padding = 10;
        int x1 = std::max(0, cell.center_x - cell.radius - padding);
        int y1 = std::max(0, cell.center_y - cell.radius - padding);
        int x2 = std::min(image.cols, cell.center_x + cell.radius + padding);
        int y2 = std::min(image.rows, cell.center_y + cell.radius + padding);
        
        if (x2 > x1 && y2 > y1) {
            cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
            cell.cellImage = image(roi).clone();
        }
        
        if (cell.area >= params.minCellArea && cell.area <= params.maxCellArea) {
            cells.append(cell);
        }
    }
    
    return cells;
}

cv::Mat AdvancedDetector::preprocessImage(const cv::Mat& input, DetectionAlgorithm algorithm) {
    cv::Mat processed;
    
    // Конвертация в градации серого, если нужно
    if (input.channels() > 1) {
        cv::cvtColor(input, processed, cv::COLOR_BGR2GRAY);
    } else {
        processed = input.clone();
    }
    
    // Специфическая предобработка для разных алгоритмов
    switch (algorithm) {
        case DetectionAlgorithm::ContourBased:
        case DetectionAlgorithm::AdaptiveThreshold:
            cv::GaussianBlur(processed, processed, cv::Size(5, 5), 0);
            break;
            
        case DetectionAlgorithm::WatershedSegmentation:
            cv::medianBlur(processed, processed, 3);
            break;
            
        case DetectionAlgorithm::MorphologicalOperations:
            cv::GaussianBlur(processed, processed, cv::Size(3, 3), 0);
            break;
            
        case DetectionAlgorithm::BlobDetection:
            // Блоб детектор работает лучше с менее размытыми изображениями
            break;
            
        default:
            break;
    }
    
    return processed;
}

Cell AdvancedDetector::createCellFromContour(const cv::Mat& source, const std::vector<cv::Point>& contour, 
                                           const std::string& imagePath) {
    Cell cell;
    
    // Вычисляем основные параметры
    cv::Moments moments = cv::moments(contour);
    if (moments.m00 != 0) {
        cell.center_x = static_cast<int>(moments.m10 / moments.m00);
        cell.center_y = static_cast<int>(moments.m01 / moments.m00);
    }
    
    cell.area = static_cast<int>(cv::contourArea(contour));
    
    // Аппроксимируем эквивалентный радиус
    cell.radius = static_cast<int>(std::sqrt(cell.area / M_PI));
    cell.diameter_pixels = cell.radius * 2;
    
    // Создаем изображение клетки
    cv::Rect boundingRect = cv::boundingRect(contour);
    int padding = 10;
    int x1 = std::max(0, boundingRect.x - padding);
    int y1 = std::max(0, boundingRect.y - padding);
    int x2 = std::min(source.cols, boundingRect.x + boundingRect.width + padding);
    int y2 = std::min(source.rows, boundingRect.y + boundingRect.height + padding);
    
    if (x2 > x1 && y2 > y1) {
        cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
        cell.cellImage = source(roi).clone();
    }
    
    cell.imagePath = imagePath;
    
    return cell;
}

std::vector<std::vector<cv::Point>> AdvancedDetector::findAndFilterContours(const cv::Mat& binary, 
                                                                           const DetectionParams& params) {
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    
    cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    // Фильтрация контуров
    std::vector<std::vector<cv::Point>> filteredContours;
    for (const auto& contour : contours) {
        double area = cv::contourArea(contour);
        double perimeter = cv::arcLength(contour, true);
        double circularity = calculateCircularity(contour);
        
        if (area >= params.minCellArea && area <= params.maxCellArea &&
            perimeter >= params.contourMinPerimeter && perimeter <= params.contourMaxPerimeter &&
            circularity >= params.minCircularity && circularity <= params.maxCircularity) {
            filteredContours.push_back(contour);
        }
    }
    
    return filteredContours;
}

double AdvancedDetector::calculateCircularity(const std::vector<cv::Point>& contour) {
    double area = cv::contourArea(contour);
    double perimeter = cv::arcLength(contour, true);
    
    if (perimeter == 0) return 0;
    
    return 4 * M_PI * area / (perimeter * perimeter);
}

double AdvancedDetector::calculateSolidity(const std::vector<cv::Point>& contour) {
    std::vector<cv::Point> hull;
    cv::convexHull(contour, hull);
    
    double contourArea = cv::contourArea(contour);
    double hullArea = cv::contourArea(hull);
    
    if (hullArea == 0) return 0;
    
    return contourArea / hullArea;
}

cv::Mat AdvancedDetector::createWatershedMarkers(const cv::Mat& image, const DetectionParams& params) {
    cv::Mat binary;
    cv::threshold(image, binary, 0, 255, cv::THRESH_BINARY_INV + cv::THRESH_OTSU);
    
    // Морфологические операции для удаления шума
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(binary, binary, cv::MORPH_OPEN, kernel, cv::Point(-1, -1), 2);
    
    // Дистанционное преобразование
    cv::Mat distTransform;
    cv::distanceTransform(binary, distTransform, cv::DIST_L2, 5);
    
    // Поиск локальных максимумов
    cv::Mat localMaxima;
    double maxVal;
    cv::minMaxLoc(distTransform, nullptr, &maxVal);
    cv::threshold(distTransform, localMaxima, 0.5 * maxVal, 255, cv::THRESH_BINARY);
    
    // Создаем маркеры
    cv::Mat markers;
    localMaxima.convertTo(markers, CV_32S);
    
    // Нумеруем связанные компоненты
    int nLabels = cv::connectedComponents(markers, markers);
    
    return markers;
}

void AdvancedDetector::removeOverlappingCells(QVector<Cell>& cells, double overlapThreshold) {
    for (int i = 0; i < cells.size(); i++) {
        for (int j = i + 1; j < cells.size(); ) {
            // Вычисляем расстояние между центрами
            double dx = cells[i].center_x - cells[j].center_x;
            double dy = cells[i].center_y - cells[j].center_y;
            double distance = std::sqrt(dx * dx + dy * dy);
            
            // Вычисляем минимальное расстояние для неперекрывающихся клеток
            double minDistance = (cells[i].radius + cells[j].radius) * (1.0 - overlapThreshold);
            
            if (distance < minDistance) {
                // Удаляем клетку с меньшей площадью
                if (cells[i].area < cells[j].area) {
                    cells.removeAt(i);
                    i--;
                    break;
                } else {
                    cells.removeAt(j);
                }
            } else {
                j++;
            }
        }
    }
}

cv::Ptr<cv::SimpleBlobDetector> AdvancedDetector::createBlobDetector(const DetectionParams& params) {
    cv::SimpleBlobDetector::Params detectorParams;
    
    // Фильтрация по площади
    detectorParams.filterByArea = true;
    detectorParams.minArea = static_cast<float>(params.minCellArea);
    detectorParams.maxArea = static_cast<float>(params.maxCellArea);
    
    // Фильтрация по круглости
    detectorParams.filterByCircularity = true;
    detectorParams.minCircularity = static_cast<float>(params.minCircularity);
    detectorParams.maxCircularity = static_cast<float>(params.maxCircularity);
    
    // Фильтрация по выпуклости
    detectorParams.filterByConvexity = true;
    detectorParams.minConvexity = 0.8f;
    
    // Фильтрация по инерции
    detectorParams.filterByInertia = false;
    
    detectorParams.minRepeatability = params.blobMinRepeatability;
    
    return cv::SimpleBlobDetector::create(detectorParams);
}

QVector<Cell> AdvancedDetector::filterCellsByParams(const QVector<Cell>& cells, const DetectionParams& params) {
    QVector<Cell> filteredCells;
    
    for (const Cell& cell : cells) {
        if (cell.area >= params.minCellArea && cell.area <= params.maxCellArea) {
            filteredCells.append(cell);
        }
    }
    
    return filteredCells;
}

QString AdvancedDetector::getAlgorithmDescription(DetectionAlgorithm algorithm) {
    switch (algorithm) {
        case DetectionAlgorithm::HoughCircles:
            return "Преобразование Хафа для окружностей";
        case DetectionAlgorithm::ContourBased:
            return "Обнаружение на основе контуров";
        case DetectionAlgorithm::WatershedSegmentation:
            return "Водораздельная сегментация";
        case DetectionAlgorithm::MorphologicalOperations:
            return "Морфологические операции";
        case DetectionAlgorithm::AdaptiveThreshold:
            return "Адаптивное пороговое значение";
        case DetectionAlgorithm::BlobDetection:
            return "Детектор блобов";
        default:
            return "Неизвестный алгоритм";
    }
}