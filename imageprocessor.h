// imageprocessor.h - IMPROVED VERSION
#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QVector>
#include <QString>
#include <QMutex>
#include "cell.h"
#include <opencv2/opencv.hpp>

class ImageProcessor {
public:
    struct HoughParams {
        double dp = 1.0;
        double minDist = 30.0;
        double param1 = 90.0;
        double param2 = 50.0;
        int minRadius = 30;
        int maxRadius = 150;
    };

    struct PreprocessingParams {
        bool adjustContrast = false;
        double contrastAlpha = 1.0;  // Contrast control (1.0-3.0)
        int contrastBeta = 0;        // Brightness control (-100 to 100)
        bool applyGaussianBlur = false;
        int gaussianKernel = 5;      // Must be odd
        bool applyBilateralFilter = false;
        int bilateralD = 9;
        double bilateralSigmaColor = 75.0;
        double bilateralSigmaSpace = 75.0;
        bool enhanceEdges = false;
        double edgeStrength = 1.0;
    };

    ImageProcessor();
    ~ImageProcessor();
    
    // Main processing function
    void processImages(const QStringList& paths, const HoughParams& params = HoughParams(), 
                      const PreprocessingParams& prepParams = PreprocessingParams());
    
    // Getters
    QVector<Cell> getDetectedCells() const { return cells; }
    QString getLastError() const;
    
    // Configuration
    void setDebugMode(bool enable);
    void setParallelProcessing(bool enable);
    void setPreprocessingParams(const PreprocessingParams& params);
    PreprocessingParams getPreprocessingParams() const { return m_preprocessingParams; }

private:
    // Processing methods
    void processImagesSequential(const QStringList& paths, const HoughParams& params, const PreprocessingParams& prepParams);
    void processImagesParallel(const QStringList& paths, const HoughParams& params, const PreprocessingParams& prepParams);
    void processSingleImage(const QString& path, const HoughParams& params, const PreprocessingParams& prepParams);
    QVector<Cell> processSingleImageThreadSafe(const QString& path, const HoughParams& params, const PreprocessingParams& prepParams);
    
    // Image preprocessing
    cv::Mat applyPreprocessing(const cv::Mat& input, const PreprocessingParams& params);
    cv::Mat loadImageSafely(const QString& imagePath);
    
    // Circle detection
    void detectCircles(const cv::Mat& blurred, std::vector<cv::Vec3f>& circles, 
                      const HoughParams& params);
    Cell createCell(const cv::Mat& src, const cv::Vec3f& circle, 
                   int padding, const std::string& imagePath);
    
    // Scale detection methods
    double detectAndCalculateScale(const cv::Mat& image);
    cv::Vec4i detectScaleLine(const cv::Mat& image);
    double calculateLineLength(const cv::Vec4i& line);
    double detectScaleValue(const cv::Mat& image, const cv::Vec4i& line);
    double detectScalePatterns(const cv::Mat& roi);
    
    // Member variables
    QVector<Cell> cells;
    QString m_lastError;
    bool m_enableParallel;
    bool m_debugMode;
    PreprocessingParams m_preprocessingParams;
    mutable QMutex m_mutex; // For thread safety
};

#endif // IMAGEPROCESSOR_H