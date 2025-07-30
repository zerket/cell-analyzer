#ifndef CELL_H
#define CELL_H

#include <opencv2/opencv.hpp>

struct Cell {
    cv::Vec3f circle;         // Координаты и радиус круга (x, y, r)
    cv::Mat image;            // Изображение клетки (вырезанный фрагмент)
    float diameterPx = 0.0f;  // Диаметр в пикселях
    float diameterNm = 0.0f;  // Диаметр в нанометрах
    int pixelDiameter = 0;
    std::string imagePath = "";
};

#endif // CELL_H
