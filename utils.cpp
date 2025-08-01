#include "utils.h"

QImage matToQImage(const cv::Mat& mat) {
    if (mat.empty()) {
        return QImage();
    }
    
    switch (mat.type()) {
    case CV_8UC1: {
        QImage img(mat.data, mat.cols, mat.rows, int(mat.step), QImage::Format_Grayscale8);
        return img.copy();
    }
    case CV_8UC3: {
        static thread_local cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        QImage img(rgb.data, rgb.cols, rgb.rows, int(rgb.step), QImage::Format_RGB888);
        return img.copy();
    }
    case CV_8UC4: {
        static thread_local cv::Mat rgba;
        cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
        QImage img(rgba.data, rgba.cols, rgba.rows, int(rgba.step), QImage::Format_RGBA8888);
        return img.copy();
    }
    default:
        return QImage();
    }
}

bool isCircleInsideImage(int x, int y, int r, int width, int height) {
    return (x - r >= 0) && (y - r >= 0) && (x + r < width) && (y + r < height);
}

double visibleCircleRatio(int x, int y, int r, int width, int height) {
    // Координаты ограничивающего квадрата круга
    int left = x - r;
    int right = x + r;
    int top = y - r;
    int bottom = y + r;

    // Координаты пересечения квадрата круга с изображением
    int visibleLeft = std::max(left, 0);
    int visibleRight = std::min(right, width - 1);
    int visibleTop = std::max(top, 0);
    int visibleBottom = std::min(bottom, height - 1);

    if (visibleRight < visibleLeft || visibleBottom < visibleTop)
        return 0.0;

    // Площадь круга
    double circleArea = CV_PI * r * r;

    // Площадь видимой части (приближенно — площадь пересечения квадратов)
    int visibleWidth = visibleRight - visibleLeft + 1;
    int visibleHeight = visibleBottom - visibleTop + 1;
    double visibleRectArea = visibleWidth * visibleHeight;

    // Коррекция: площадь пересечения круга и изображения меньше площади пересечения квадратов,
    // но для оценки достаточно приближения.

    // Если круг почти целиком внутри, visibleRectArea ~ circleArea
    // Для более точной оценки можно считать пиксели внутри круга, но это дороже.

    return visibleRectArea / circleArea;
}
