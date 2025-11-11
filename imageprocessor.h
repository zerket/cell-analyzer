// imageprocessor.h - ONNX-BASED VERSION
#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QVector>
#include <QString>
#include <QMutex>
#include "cell.h"
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

class ImageProcessor {
public:
    struct YoloParams {
        QString modelPath = "ml-data/models/yolov8s_cells_v1.0.onnx";
        double confThreshold = 0.25;
        double iouThreshold = 0.7;
        int minCellArea = 500;
        bool useCUDA = false;  // Use CUDA backend if available
    };

    ImageProcessor();
    ~ImageProcessor();

    // Main processing function using YOLO
    void processImages(const QStringList& paths, const YoloParams& params = YoloParams());

    // Getters
    QVector<Cell> getDetectedCells() const { return cells; }
    QString getLastError() const;

    // Configuration
    void setDebugMode(bool enable);

private:
    // Processing methods
    void processSingleImage(const QString& path, const YoloParams& params);
    QVector<Cell> detectCellsWithONNX(const QString& imagePath, const YoloParams& params);

    // ONNX inference helpers
    cv::Mat preprocessImage(const cv::Mat& image);
    QVector<Cell> postprocessONNX(const cv::Mat& output, const cv::Mat& originalImage,
                                   const QString& imagePath, const YoloParams& params);

    // Image loading
    cv::Mat loadImageSafely(const QString& imagePath);

    // Scale detection methods (keep for μm conversion)
    double detectAndCalculateScale(const cv::Mat& image);
    cv::Vec4i detectScaleLine(const cv::Mat& image);
    double calculateLineLength(const cv::Vec4i& line);
    double detectScaleValue(const cv::Mat& image, const cv::Vec4i& line);
    double detectScalePatterns(const cv::Mat& roi);

    // Member variables
    QVector<Cell> cells;
    QString m_lastError;
    bool m_debugMode;
    mutable QMutex m_mutex;
};

#endif // IMAGEPROCESSOR_H
