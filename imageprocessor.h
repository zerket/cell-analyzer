// imageprocessor.h - YOLO-BASED VERSION
#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QVector>
#include <QString>
#include <QMutex>
#include "cell.h"
#include <opencv2/opencv.hpp>

class ImageProcessor {
public:
    struct YoloParams {
        QString modelPath = "ml-data/models/yolov8s_cells_v1.0.pt";
        double confThreshold = 0.25;
        double iouThreshold = 0.7;
        int minCellArea = 500;
        QString device = "0";  // "0" for GPU, "cpu" for CPU
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
    QVector<Cell> detectCellsWithYolo(const QString& imagePath, const YoloParams& params);
    Cell createCellFromYoloDetection(const cv::Mat& srcImage, const QJsonObject& cellData,
                                     const std::string& imagePath);

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
