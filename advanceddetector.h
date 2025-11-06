#ifndef ADVANCEDDETECTOR_H
#define ADVANCEDDETECTOR_H

#include <opencv2/opencv.hpp>
#include <QVector>
#include "cell.h"

class AdvancedDetector {
public:
    enum class DetectionAlgorithm {
        HoughCircles,       // Существующий алгоритм (круглые клетки)
        ContourBased,       // Алгоритм на основе контуров (произвольные формы)
        WatershedSegmentation, // Водораздельная сегментация
        MorphologicalOperations, // Морфологические операции
        AdaptiveThreshold,  // Адаптивное пороговое значение
        BlobDetection,      // Детектор блобов
        NeuralNetwork       // Нейросетевая детекция (U-Net, ONNX)
    };

    struct DetectionParams {
        DetectionAlgorithm algorithm = DetectionAlgorithm::HoughCircles;
        
        // Общие параметры
        int minCellArea = 500;      // Минимальная площадь клетки в пикселях
        int maxCellArea = 15000;    // Максимальная площадь клетки в пикселях
        double minCircularity = 0.3; // Минимальная круглость (0-1)
        double maxCircularity = 1.0; // Максимальная круглость
        
        // Параметры для ContourBased
        double contourMinPerimeter = 50.0;
        double contourMaxPerimeter = 800.0;
        double contourApproxEpsilon = 0.02; // Для аппроксимации контура
        
        // Параметры для Watershed
        int watershedMarkers = 0;    // 0 - автоматическое определение
        double watershedMinDistance = 20.0;
        
        // Параметры для MorphologicalOperations
        int morphKernelSize = 5;
        int morphIterations = 2;
        cv::MorphShapes morphShape = cv::MORPH_ELLIPSE;
        
        // Параметры для AdaptiveThreshold
        int adaptiveBlockSize = 11;
        double adaptiveC = 2.0;
        cv::AdaptiveThresholdTypes adaptiveMethod = cv::ADAPTIVE_THRESH_GAUSSIAN_C;
        
        // Параметры для BlobDetection
        float blobMinThreshold = 50.0f;
        float blobMaxThreshold = 220.0f;
        float blobThresholdStep = 10.0f;
        size_t blobMinRepeatability = 2;

        // Параметры для NeuralNetwork (хранится указатель для передачи в NeuralNetDetector)
        void* neuralNetParams = nullptr; // Указатель на NeuralNetDetector::NeuralNetParams
    };

    AdvancedDetector();
    ~AdvancedDetector();

    // Основная функция обнаружения
    QVector<Cell> detectCells(const cv::Mat& image, const DetectionParams& params);
    
    // Специализированные методы обнаружения
    QVector<Cell> detectWithContours(const cv::Mat& image, const DetectionParams& params);
    QVector<Cell> detectWithWatershed(const cv::Mat& image, const DetectionParams& params);
    QVector<Cell> detectWithMorphology(const cv::Mat& image, const DetectionParams& params);
    QVector<Cell> detectWithAdaptiveThreshold(const cv::Mat& image, const DetectionParams& params);
    QVector<Cell> detectWithBlobDetector(const cv::Mat& image, const DetectionParams& params);
    QVector<Cell> detectWithNeuralNetwork(const cv::Mat& image, const DetectionParams& params);
    
    // Вспомогательные функции
    static double calculateCircularity(const std::vector<cv::Point>& contour);
    static double calculateSolidity(const std::vector<cv::Point>& contour);
    static cv::Rect getBoundingRect(const std::vector<cv::Point>& contour);
    static cv::Mat createCellImage(const cv::Mat& source, const std::vector<cv::Point>& contour, int padding = 10);
    
    // Фильтрация результатов
    QVector<Cell> filterCellsByParams(const QVector<Cell>& cells, const DetectionParams& params);
    
    // Получение описания алгоритма
    static QString getAlgorithmDescription(DetectionAlgorithm algorithm);
    
private:
    // Предобработка изображения
    cv::Mat preprocessImage(const cv::Mat& input, DetectionAlgorithm algorithm);
    
    // Создание Cell объекта из контура
    Cell createCellFromContour(const cv::Mat& source, const std::vector<cv::Point>& contour, 
                              const std::string& imagePath);
    
    // Вспомогательные функции для различных алгоритмов
    std::vector<std::vector<cv::Point>> findAndFilterContours(const cv::Mat& binary, 
                                                             const DetectionParams& params);
    cv::Mat createWatershedMarkers(const cv::Mat& image, const DetectionParams& params);
    void removeOverlappingCells(QVector<Cell>& cells, double overlapThreshold = 0.3);
    
    // Блоб детектор
    cv::Ptr<cv::SimpleBlobDetector> createBlobDetector(const DetectionParams& params);

    // Нейросетевой детектор
    class NeuralNetDetector* m_neuralDetector;
};

#endif // ADVANCEDDETECTOR_H