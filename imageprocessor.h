#ifndef IMAGEPROCESSOR_H
#define IMAGEPROCESSOR_H

#include <QVector>
#include <QString>
#include "cell.h"
#include <opencv2/opencv.hpp>

class ImageProcessor {
public:
    void processImages(const QStringList& paths);
    QVector<Cell> getDetectedCells() const { return cells; }

private:
    QVector<Cell> cells;
    cv::Vec4i detectScaleLine(const cv::Mat& image);
    float calculateScale(const cv::Vec4i& line, const std::string& text);
    std::string detectScaleText(const cv::Mat& image, const cv::Vec4i& line);
};

#endif // IMAGEPROCESSOR_H
