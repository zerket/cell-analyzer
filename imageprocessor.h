#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QVector>
#include <QString>
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

    void processImages(const QStringList& paths, const HoughParams& params = HoughParams());
    QVector<Cell> getDetectedCells() const { return cells; }

private:
    QVector<Cell> cells;
    cv::Vec4i detectScaleLine(const cv::Mat& image);
    float calculateScale(const cv::Vec4i& line, const std::string& text);
    std::string detectScaleText(const cv::Mat& image, const cv::Vec4i& line);
};

#endif // IMAGEPROCESSOR_H
