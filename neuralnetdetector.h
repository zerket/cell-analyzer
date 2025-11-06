#ifndef NEURALNETDETECTOR_H
#define NEURALNETDETECTOR_H

#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#include <QVector>
#include <QMap>
#include <QString>
#include <string>
#include <vector>
#include "cell.h"

/**
 * @brief NeuralNetDetector - класс для детекции клеток с использованием нейронных сетей
 *
 * Поддерживает:
 * - Загрузка ONNX моделей
 * - Мультиклассовая сегментация (несколько типов клеток)
 * - CPU и CUDA инференс
 * - Постобработка масок в Cell объекты
 */
class NeuralNetDetector {
public:
    struct NeuralNetParams {
        std::string modelPath = "";          // Путь к ONNX модели
        int inputSize = 512;                 // Размер входного изображения (512x512, 1024x1024)
        float confidenceThreshold = 0.5f;    // Порог уверенности (0.0-1.0)
        int minCellSize = 50;                // Минимальный размер клетки в пикселях
        int maxCellSize = 1000;              // Максимальный размер клетки в пикселях
        float nmsThreshold = 0.3f;           // IoU threshold для Non-Maximum Suppression
        bool useGPU = false;                 // Использовать GPU (CUDA) если доступно
        int numClasses = 3;                  // Количество классов клеток (не включая фон)

        // Маппинг классов: classId -> className
        QMap<int, QString> classNames;       // Например: {1: "Type A", 2: "Type B", 3: "Type C"}

        // Параметры постобработки
        bool fillHoles = true;               // Заполнять дырки в масках
        int morphKernelSize = 3;             // Размер ядра для морфологических операций
        double minCircularity = 0.0;         // Минимальная круглость (0-1, 0 = отключено)
        double maxCircularity = 1.0;         // Максимальная круглость
    };

    NeuralNetDetector();
    ~NeuralNetDetector();

    /**
     * @brief Загрузка модели из ONNX файла
     * @param modelPath Путь к .onnx файлу
     * @param useGPU Использовать GPU если доступно
     * @return true если модель успешно загружена
     */
    bool loadModel(const std::string& modelPath, bool useGPU = false);

    /**
     * @brief Проверка загружена ли модель
     */
    bool isModelLoaded() const;

    /**
     * @brief Получить информацию о модели
     */
    QString getModelInfo() const;

    /**
     * @brief Основная функция детекции клеток
     * @param image Входное изображение
     * @param params Параметры детекции
     * @return Вектор обнаруженных клеток с типами и уверенностью
     */
    QVector<Cell> detectCells(const cv::Mat& image, const NeuralNetParams& params);

    /**
     * @brief Получить список доступных GPU (CUDA devices)
     */
    static QVector<QString> getAvailableGPUs();

    /**
     * @brief Проверить доступность CUDA
     */
    static bool isCudaAvailable();

private:
    /**
     * @brief Предобработка изображения для нейросети
     * @param input Входное изображение
     * @param targetSize Целевой размер (например, 512)
     * @return Предобработанное изображение и коэффициенты масштабирования
     */
    struct PreprocessResult {
        cv::Mat processedImage;
        float scaleX;
        float scaleY;
        int paddingTop;
        int paddingLeft;
    };
    PreprocessResult preprocessImage(const cv::Mat& input, int targetSize);

    /**
     * @brief Постобработка выходной маски сети
     * @param outputMask Выходная маска сети (C x H x W или 1 x C x H x W)
     * @param originalImage Оригинальное изображение
     * @param preprocessResult Результат предобработки (для обратного масштабирования)
     * @param params Параметры детекции
     * @return Вектор обнаруженных клеток
     */
    QVector<Cell> postprocessMask(const cv::Mat& outputMask,
                                   const cv::Mat& originalImage,
                                   const PreprocessResult& preprocessResult,
                                   const NeuralNetParams& params);

    /**
     * @brief Извлечение клеток из бинарной маски одного класса
     * @param classMask Бинарная маска класса
     * @param classId ID класса
     * @param className Название класса
     * @param originalImage Оригинальное изображение
     * @param params Параметры детекции
     * @return Вектор клеток данного класса
     */
    QVector<Cell> extractCellsFromMask(const cv::Mat& classMask,
                                        int classId,
                                        const QString& className,
                                        const cv::Mat& originalImage,
                                        const NeuralNetParams& params);

    /**
     * @brief Создание Cell объекта из контура
     * @param contour Контур клетки
     * @param classId ID класса
     * @param className Название класса
     * @param confidence Уверенность детекции
     * @param originalImage Оригинальное изображение
     * @return Cell объект
     */
    Cell createCellFromContour(const std::vector<cv::Point>& contour,
                               int classId,
                               const QString& className,
                               float confidence,
                               const cv::Mat& originalImage);

    /**
     * @brief Non-Maximum Suppression для удаления перекрывающихся детекций
     * @param cells Вектор клеток
     * @param nmsThreshold IoU порог
     * @return Отфильтрованный вектор клеток
     */
    QVector<Cell> applyNMS(const QVector<Cell>& cells, float nmsThreshold);

    /**
     * @brief Вычисление IoU (Intersection over Union) между двумя клетками
     */
    float calculateIoU(const Cell& cell1, const Cell& cell2);

    /**
     * @brief Морфологическая постобработка маски
     */
    cv::Mat morphologicalPostprocess(const cv::Mat& mask, int kernelSize, bool fillHoles);

private:
    cv::dnn::Net m_model;           // OpenCV DNN модель
    bool m_modelLoaded;             // Флаг загрузки модели
    std::string m_modelPath;        // Путь к модели
    bool m_usingGPU;                // Используется ли GPU
    int m_inputSize;                // Размер входа модели
};

#endif // NEURALNETDETECTOR_H
