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
    
    // Default constructor
    Cell() = default;
    
    // Copy constructor with deep copy of cv::Mat
    Cell(const Cell& other) 
        : circle(other.circle)
        , diameterPx(other.diameterPx)
        , diameterNm(other.diameterNm)
        , pixelDiameter(other.pixelDiameter)
        , imagePath(other.imagePath)
    {
        if (!other.image.empty()) {
            image = other.image.clone();
        }
    }
    
    // Assignment operator with deep copy of cv::Mat
    Cell& operator=(const Cell& other) {
        if (this != &other) {
            circle = other.circle;
            diameterPx = other.diameterPx;
            diameterNm = other.diameterNm;
            pixelDiameter = other.pixelDiameter;
            imagePath = other.imagePath;
            if (!other.image.empty()) {
                image = other.image.clone();
            } else {
                image = cv::Mat();
            }
        }
        return *this;
    }
};

#endif // CELL_H
