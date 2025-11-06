#include "neuralnetdetector.h"
#include "logger.h"
#include <QFileInfo>
#include <algorithm>
#include <numeric>

NeuralNetDetector::NeuralNetDetector()
    : m_modelLoaded(false)
    , m_usingGPU(false)
    , m_inputSize(512)
{
}

NeuralNetDetector::~NeuralNetDetector() {
}

bool NeuralNetDetector::loadModel(const std::string& modelPath, bool useGPU) {
    try {
        LOG_INFO("Loading neural network model from: " + QString::fromStdString(modelPath));

        // Проверка существования файла
        QFileInfo fileInfo(QString::fromStdString(modelPath));
        if (!fileInfo.exists()) {
            LOG_ERROR("Model file does not exist: " + QString::fromStdString(modelPath));
            return false;
        }

        // Загрузка ONNX модели
        m_model = cv::dnn::readNetFromONNX(modelPath);

        if (m_model.empty()) {
            LOG_ERROR("Failed to load model from: " + QString::fromStdString(modelPath));
            return false;
        }

        // Настройка бэкенда
        if (useGPU && isCudaAvailable()) {
            LOG_INFO("Attempting to use CUDA backend");
            m_model.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_model.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
            m_usingGPU = true;
            LOG_INFO("CUDA backend enabled successfully");
        } else {
            LOG_INFO("Using CPU backend");
            m_model.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_model.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            m_usingGPU = false;
        }

        m_modelPath = modelPath;
        m_modelLoaded = true;

        LOG_INFO("Model loaded successfully");
        return true;
    }
    catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV exception while loading model: " + QString::fromStdString(e.what()));
        m_modelLoaded = false;
        return false;
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception while loading model: " + QString::fromStdString(e.what()));
        m_modelLoaded = false;
        return false;
    }
}

bool NeuralNetDetector::isModelLoaded() const {
    return m_modelLoaded;
}

QString NeuralNetDetector::getModelInfo() const {
    if (!m_modelLoaded) {
        return "No model loaded";
    }

    QString info = QString("Model: %1\n").arg(QString::fromStdString(m_modelPath));
    info += QString("Backend: %1\n").arg(m_usingGPU ? "CUDA (GPU)" : "CPU");
    info += QString("Input size: %1x%1").arg(m_inputSize);

    return info;
}

QVector<Cell> NeuralNetDetector::detectCells(const cv::Mat& image, const NeuralNetParams& params) {
    if (!m_modelLoaded) {
        LOG_ERROR("Cannot detect cells: model not loaded");
        return QVector<Cell>();
    }

    if (image.empty()) {
        LOG_ERROR("Cannot detect cells: input image is empty");
        return QVector<Cell>();
    }

    try {
        LOG_INFO(QString("Starting neural network detection on image %1x%2")
                 .arg(image.cols).arg(image.rows));

        // 1. Предобработка изображения
        PreprocessResult preprocessResult = preprocessImage(image, params.inputSize);

        // 2. Создание blob для сети
        cv::Mat blob = cv::dnn::blobFromImage(
            preprocessResult.processedImage,
            1.0 / 255.0,  // Нормализация [0, 255] -> [0, 1]
            cv::Size(params.inputSize, params.inputSize),
            cv::Scalar(0, 0, 0),
            true,  // swapRB (BGR -> RGB)
            false  // crop
        );

        LOG_DEBUG(QString("Created blob with shape: %1x%2x%3x%4")
                  .arg(blob.size[0]).arg(blob.size[1]).arg(blob.size[2]).arg(blob.size[3]));

        // 3. Инференс
        m_model.setInput(blob);
        cv::Mat output = m_model.forward();

        LOG_DEBUG(QString("Network output shape: %1 dimensions").arg(output.dims));
        for (int i = 0; i < output.dims; i++) {
            LOG_DEBUG(QString("  Dimension %1: %2").arg(i).arg(output.size[i]));
        }

        // 4. Постобработка
        QVector<Cell> cells = postprocessMask(output, image, preprocessResult, params);

        LOG_INFO(QString("Neural network detection completed: %1 cells found").arg(cells.size()));

        return cells;
    }
    catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV exception during detection: " + QString::fromStdString(e.what()));
        return QVector<Cell>();
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception during detection: " + QString::fromStdString(e.what()));
        return QVector<Cell>();
    }
}

NeuralNetDetector::PreprocessResult NeuralNetDetector::preprocessImage(const cv::Mat& input, int targetSize) {
    PreprocessResult result;

    // Конвертация в BGR если нужно
    cv::Mat inputBGR;
    if (input.channels() == 1) {
        cv::cvtColor(input, inputBGR, cv::COLOR_GRAY2BGR);
    } else if (input.channels() == 4) {
        cv::cvtColor(input, inputBGR, cv::COLOR_BGRA2BGR);
    } else {
        inputBGR = input.clone();
    }

    // Вычисление коэффициентов масштабирования
    result.scaleX = static_cast<float>(inputBGR.cols) / targetSize;
    result.scaleY = static_cast<float>(inputBGR.rows) / targetSize;

    // Resize с сохранением пропорций (letterbox)
    float scale = std::min(
        static_cast<float>(targetSize) / inputBGR.cols,
        static_cast<float>(targetSize) / inputBGR.rows
    );

    int newWidth = static_cast<int>(inputBGR.cols * scale);
    int newHeight = static_cast<int>(inputBGR.rows * scale);

    cv::Mat resized;
    cv::resize(inputBGR, resized, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_LINEAR);

    // Создание изображения с padding
    result.processedImage = cv::Mat(targetSize, targetSize, CV_8UC3, cv::Scalar(114, 114, 114));

    result.paddingTop = (targetSize - newHeight) / 2;
    result.paddingLeft = (targetSize - newWidth) / 2;

    // Копирование resized изображения в центр
    cv::Rect roi(result.paddingLeft, result.paddingTop, newWidth, newHeight);
    resized.copyTo(result.processedImage(roi));

    LOG_DEBUG(QString("Preprocessed image: %1x%2 -> %3x%3 (scale: %4, padding: %5x%6)")
              .arg(input.cols).arg(input.rows)
              .arg(targetSize)
              .arg(scale)
              .arg(result.paddingLeft).arg(result.paddingTop));

    return result;
}

QVector<Cell> NeuralNetDetector::postprocessMask(const cv::Mat& outputMask,
                                                   const cv::Mat& originalImage,
                                                   const PreprocessResult& preprocessResult,
                                                   const NeuralNetParams& params) {
    QVector<Cell> allCells;

    try {
        // Output обычно имеет форму: [1, num_classes, H, W] или [num_classes, H, W]
        cv::Mat output = outputMask;

        // Если 4D [1, C, H, W], берем [0]
        if (output.dims == 4) {
            std::vector<cv::Range> ranges = {cv::Range(0, 1), cv::Range::all(), cv::Range::all(), cv::Range::all()};
            output = output(ranges).clone();
            output = output.reshape(0, {output.size[1], output.size[2], output.size[3]});
        }

        // Теперь output должен быть [C, H, W]
        if (output.dims != 3) {
            LOG_ERROR(QString("Unexpected output dimensions: %1").arg(output.dims));
            return allCells;
        }

        int numClasses = output.size[0];
        int maskHeight = output.size[1];
        int maskWidth = output.size[2];

        LOG_DEBUG(QString("Processing mask: %1 classes, %2x%3")
                  .arg(numClasses).arg(maskHeight).arg(maskWidth));

        // Обработка каждого класса (пропускаем класс 0 = background)
        for (int classId = 1; classId <= std::min(numClasses - 1, params.numClasses); classId++) {
            // Извлечение маски класса
            cv::Mat classMask(maskHeight, maskWidth, CV_32F, output.ptr<float>(classId));

            // Применение порога уверенности
            cv::Mat binaryMask;
            cv::threshold(classMask, binaryMask, params.confidenceThreshold, 1.0, cv::THRESH_BINARY);
            binaryMask.convertTo(binaryMask, CV_8U, 255.0);

            // Морфологическая постобработка
            if (params.fillHoles || params.morphKernelSize > 0) {
                binaryMask = morphologicalPostprocess(binaryMask, params.morphKernelSize, params.fillHoles);
            }

            // Resize маски обратно к оригинальному размеру
            // Сначала убираем padding
            int originalWidth = params.inputSize - 2 * preprocessResult.paddingLeft;
            int originalHeight = params.inputSize - 2 * preprocessResult.paddingTop;

            cv::Rect roiRect(
                preprocessResult.paddingLeft * maskWidth / params.inputSize,
                preprocessResult.paddingTop * maskHeight / params.inputSize,
                originalWidth * maskWidth / params.inputSize,
                originalHeight * maskHeight / params.inputSize
            );

            if (roiRect.x >= 0 && roiRect.y >= 0 &&
                roiRect.x + roiRect.width <= maskWidth &&
                roiRect.y + roiRect.height <= maskHeight) {

                cv::Mat croppedMask = binaryMask(roiRect);
                cv::Mat resizedMask;
                cv::resize(croppedMask, resizedMask, originalImage.size(), 0, 0, cv::INTER_NEAREST);

                // Извлечение клеток из маски
                QString className = params.classNames.value(classId, QString("Class %1").arg(classId));
                QVector<Cell> classCells = extractCellsFromMask(resizedMask, classId, className, originalImage, params);

                LOG_DEBUG(QString("Class %1 (%2): found %3 cells")
                          .arg(classId).arg(className).arg(classCells.size()));

                allCells.append(classCells);
            }
        }

        // Применение NMS для удаления перекрывающихся детекций
        if (params.nmsThreshold > 0 && allCells.size() > 1) {
            allCells = applyNMS(allCells, params.nmsThreshold);
            LOG_DEBUG(QString("After NMS: %1 cells").arg(allCells.size()));
        }

    }
    catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV exception in postprocessMask: " + QString::fromStdString(e.what()));
    }
    catch (const std::exception& e) {
        LOG_ERROR("Exception in postprocessMask: " + QString::fromStdString(e.what()));
    }

    return allCells;
}

QVector<Cell> NeuralNetDetector::extractCellsFromMask(const cv::Mat& classMask,
                                                        int classId,
                                                        const QString& className,
                                                        const cv::Mat& originalImage,
                                                        const NeuralNetParams& params) {
    QVector<Cell> cells;

    // Поиск контуров
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(classMask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (const auto& contour : contours) {
        // Фильтрация по размеру
        double area = cv::contourArea(contour);
        if (area < params.minCellSize || area > params.maxCellSize) {
            continue;
        }

        // Фильтрация по круглости
        if (params.minCircularity > 0 || params.maxCircularity < 1.0) {
            double perimeter = cv::arcLength(contour, true);
            double circularity = 4.0 * CV_PI * area / (perimeter * perimeter);

            if (circularity < params.minCircularity || circularity > params.maxCircularity) {
                continue;
            }
        }

        // Вычисление средней уверенности для данного контура
        cv::Mat mask = cv::Mat::zeros(classMask.size(), CV_8U);
        cv::drawContours(mask, std::vector<std::vector<cv::Point>>{contour}, 0, cv::Scalar(255), -1);

        float confidence = params.confidenceThreshold; // По умолчанию используем порог
        // Можно вычислить среднюю уверенность, если нужно

        // Создание Cell объекта
        Cell cell = createCellFromContour(contour, classId, className, confidence, originalImage);
        cells.push_back(cell);
    }

    return cells;
}

Cell NeuralNetDetector::createCellFromContour(const std::vector<cv::Point>& contour,
                                               int classId,
                                               const QString& className,
                                               float confidence,
                                               const cv::Mat& originalImage) {
    Cell cell;

    // Вычисление минимального описывающего круга
    cv::Point2f center;
    float radius;
    cv::minEnclosingCircle(contour, center, radius);

    cell.center_x = static_cast<int>(center.x);
    cell.center_y = static_cast<int>(center.y);
    cell.radius = static_cast<int>(radius);
    cell.diameter_pixels = static_cast<int>(radius * 2);
    cell.area = static_cast<int>(cv::contourArea(contour));

    // Информация о классе
    cell.cellType = classId;
    cell.cellTypeName = className.toStdString();
    cell.confidence = confidence;

    // Для совместимости
    cell.circle = cv::Vec3f(center.x, center.y, radius);
    cell.diameterPx = radius * 2;
    cell.pixelDiameter = static_cast<int>(radius * 2);

    // Извлечение изображения клетки
    cv::Rect boundingBox = cv::boundingRect(contour);

    // Проверка границ
    boundingBox = boundingBox & cv::Rect(0, 0, originalImage.cols, originalImage.rows);

    if (boundingBox.width > 0 && boundingBox.height > 0) {
        cell.cellImage = originalImage(boundingBox).clone();
        cell.image = cell.cellImage.clone();
    }

    return cell;
}

QVector<Cell> NeuralNetDetector::applyNMS(const QVector<Cell>& cells, float nmsThreshold) {
    if (cells.size() <= 1) {
        return cells;
    }

    // Сортировка по уверенности (по убыванию)
    QVector<Cell> sortedCells = cells;
    std::sort(sortedCells.begin(), sortedCells.end(),
              [](const Cell& a, const Cell& b) { return a.confidence > b.confidence; });

    QVector<bool> suppressed(sortedCells.size(), false);
    QVector<Cell> result;

    for (int i = 0; i < sortedCells.size(); i++) {
        if (suppressed[i]) {
            continue;
        }

        result.push_back(sortedCells[i]);

        // Подавление перекрывающихся детекций
        for (int j = i + 1; j < sortedCells.size(); j++) {
            if (suppressed[j]) {
                continue;
            }

            float iou = calculateIoU(sortedCells[i], sortedCells[j]);
            if (iou > nmsThreshold) {
                suppressed[j] = true;
            }
        }
    }

    return result;
}

float NeuralNetDetector::calculateIoU(const Cell& cell1, const Cell& cell2) {
    // Упрощенный расчет IoU для кругов
    float dx = cell1.center_x - cell2.center_x;
    float dy = cell1.center_y - cell2.center_y;
    float distance = std::sqrt(dx * dx + dy * dy);

    float r1 = cell1.radius;
    float r2 = cell2.radius;

    // Если круги не пересекаются
    if (distance >= r1 + r2) {
        return 0.0f;
    }

    // Если один круг внутри другого
    if (distance <= std::abs(r1 - r2)) {
        float minArea = CV_PI * std::min(r1, r2) * std::min(r1, r2);
        float maxArea = CV_PI * std::max(r1, r2) * std::max(r1, r2);
        return minArea / maxArea;
    }

    // Площади кругов
    float area1 = CV_PI * r1 * r1;
    float area2 = CV_PI * r2 * r2;

    // Площадь пересечения (приближенная формула)
    float part1 = r1 * r1 * std::acos((distance * distance + r1 * r1 - r2 * r2) / (2 * distance * r1));
    float part2 = r2 * r2 * std::acos((distance * distance + r2 * r2 - r1 * r1) / (2 * distance * r2));
    float part3 = 0.5f * std::sqrt((-distance + r1 + r2) * (distance + r1 - r2) *
                                    (distance - r1 + r2) * (distance + r1 + r2));

    float intersection = part1 + part2 - part3;
    float unionArea = area1 + area2 - intersection;

    return intersection / unionArea;
}

cv::Mat NeuralNetDetector::morphologicalPostprocess(const cv::Mat& mask, int kernelSize, bool fillHoles) {
    cv::Mat result = mask.clone();

    if (kernelSize > 0) {
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE,
                                                    cv::Size(kernelSize, kernelSize));

        // Closing для заполнения маленьких дырок
        cv::morphologyEx(result, result, cv::MORPH_CLOSE, kernel);

        // Opening для удаления шума
        cv::morphologyEx(result, result, cv::MORPH_OPEN, kernel);
    }

    if (fillHoles) {
        // Заполнение дырок через flood fill от границ
        cv::Mat floodFilled = result.clone();
        cv::floodFill(floodFilled, cv::Point(0, 0), cv::Scalar(255));
        cv::Mat floodFilledInv;
        cv::bitwise_not(floodFilled, floodFilledInv);
        result = (result | floodFilledInv);
    }

    return result;
}

QVector<QString> NeuralNetDetector::getAvailableGPUs() {
    QVector<QString> gpus;

#ifdef HAVE_CUDA
    try {
        int deviceCount = cv::cuda::getCudaEnabledDeviceCount();
        for (int i = 0; i < deviceCount; i++) {
            cv::cuda::DeviceInfo deviceInfo(i);
            gpus.push_back(QString::fromStdString(deviceInfo.name()));
        }
    }
    catch (...) {
        // CUDA not available
    }
#endif

    if (gpus.isEmpty()) {
        gpus.push_back("No CUDA devices found");
    }

    return gpus;
}

bool NeuralNetDetector::isCudaAvailable() {
#ifdef HAVE_CUDA
    try {
        return cv::cuda::getCudaEnabledDeviceCount() > 0;
    }
    catch (...) {
        return false;
    }
#else
    return false;
#endif
}
