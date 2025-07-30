#ifndef UTILS_H
#define UTILS_H

#include <QImage>
#include <opencv2/opencv.hpp>

QImage matToQImage(const cv::Mat& mat);
bool isCircleInsideImage(int x, int y, int r, int width, int height);
double visibleCircleRatio(int x, int y, int r, int width, int height);

#endif // UTILS_H
