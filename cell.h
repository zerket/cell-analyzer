#ifndef CELL_H
#define CELL_H

#include <opencv2/opencv.hpp>

struct Cell {
    // Основные параметры
    int center_x = 0;         // X координата центра
    int center_y = 0;         // Y координата центра
    int radius = 0;           // Радиус в пикселях
    int diameter_pixels = 0;  // Диаметр в пикселях
    double diameter_nm = 0.0; // Диаметр в нанометрах
    int area = 0;             // Площадь в пикселях

    // Параметры для нейросетевой детекции
    int cellType = 0;         // Тип клетки (0 = unknown/not classified, 1+ = class ID)
    std::string cellTypeName = ""; // Название типа клетки (например, "Type A", "Type B")
    float confidence = 1.0f;  // Уверенность детекции (0.0-1.0), для традиционных алгоритмов = 1.0

    // Изображение и путь
    cv::Mat cellImage;        // Изображение клетки (вырезанный фрагмент)
    std::string imagePath = "";
    
    // Совместимость с существующим кодом
    cv::Vec3f circle;         // Координаты и радиус круга (x, y, r)
    cv::Mat image;            // Изображение клетки (для совместимости)
    float diameterPx = 0.0f;  // Диаметр в пикселях (для совместимости)
    float diameterNm = 0.0f;  // Диаметр в нанометрах (для совместимости)
    int pixelDiameter = 0;    // Для совместимости
    
    // Default constructor
    Cell() = default;
    
    // Copy constructor with deep copy of cv::Mat
    Cell(const Cell& other)
        : center_x(other.center_x)
        , center_y(other.center_y)
        , radius(other.radius)
        , diameter_pixels(other.diameter_pixels)
        , diameter_nm(other.diameter_nm)
        , area(other.area)
        , cellType(other.cellType)
        , cellTypeName(other.cellTypeName)
        , confidence(other.confidence)
        , imagePath(other.imagePath)
        , circle(other.circle)
        , diameterPx(other.diameterPx)
        , diameterNm(other.diameterNm)
        , pixelDiameter(other.pixelDiameter)
    {
        if (!other.cellImage.empty()) {
            cellImage = other.cellImage.clone();
        }
        if (!other.image.empty()) {
            image = other.image.clone();
        }
    }
    
    // Assignment operator with deep copy of cv::Mat
    Cell& operator=(const Cell& other) {
        if (this != &other) {
            center_x = other.center_x;
            center_y = other.center_y;
            radius = other.radius;
            diameter_pixels = other.diameter_pixels;
            diameter_nm = other.diameter_nm;
            area = other.area;
            cellType = other.cellType;
            cellTypeName = other.cellTypeName;
            confidence = other.confidence;
            imagePath = other.imagePath;
            circle = other.circle;
            diameterPx = other.diameterPx;
            diameterNm = other.diameterNm;
            pixelDiameter = other.pixelDiameter;
            
            if (!other.cellImage.empty()) {
                cellImage = other.cellImage.clone();
            } else {
                cellImage = cv::Mat();
            }
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
