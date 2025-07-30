#include "imageprocessor.h"
#include "utils.h"
#include <cctype>
#include <string>

void ImageProcessor::processImages(const QStringList& paths) {
    cells.clear();
    for (const QString& path : paths) {
        cv::Mat src = cv::imread(path.toStdString());
        if (src.empty()) continue;

        cv::Mat gray, blurred;
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
        cv::medianBlur(gray, blurred, 5);

        cv::Mat srcCopy = src.clone();

        std::vector<cv::Vec3f> circles;

        cv::HoughCircles(
            blurred, circles,
            cv::HOUGH_GRADIENT,
            1,      // dp — разрешение аккумулятора (оставьте 1)
            30,     // minDist — минимальное расстояние между центрами (уменьшить с 50 до 30, чтобы не пропускать близко расположенные клетки)
            80,     // param1 — порог для Canny (увеличить с 100 до 120 для более чётких границ)
            40,     // param2 — порог для центра круга (увеличить с 30 до 40-50, чтобы уменьшить ложные срабатывания)
            30,     // minRadius — минимальный радиус (оставьте 10)
            130     // maxRadius — максимальный радиус (уменьшить с 50 до 60, подберите под размер клеток)
        );

        int padding = 30; // паддинг для вырезания клетки

        for (const auto& c : circles) {
            int x = cvRound(c[0]);
            int y = cvRound(c[1]);
            int r = cvRound(c[2]);

            double visibleRatio = visibleCircleRatio(x, y, r, src.cols, src.rows);

            if (visibleRatio < 0.6) {
                // Меньше 60% видимой окружности — пропускаем
                continue;
            }

            // Рисуем красный прямоугольник без паддинга (точно вписанный в окружность)
            cv::Rect rectForDrawing(x - r, y - r, 2 * r, 2 * r);
            cv::rectangle(srcCopy, rectForDrawing, cv::Scalar(0, 0, 255), 2);

            // Формируем ROI с паддингом для вырезания клетки (с запасом)
            int padding = 30;
            int roiX = std::max(x - r - padding, 0);
            int roiY = std::max(y - r - padding, 0);
            int roiW = std::min(2 * r + 2 * padding, src.cols - roiX);
            int roiH = std::min(2 * r + 2 * padding, src.rows - roiY);
            cv::Rect rectForCrop(roiX, roiY, roiW, roiH);

            Cell cell;
            cell.circle = c;
            cell.image = src(rectForCrop).clone(); // с паддингом для превью
            cell.diameterPx = 2 * r;
            cells.append(cell);
        }

        cv::imwrite("debug_cells_highlighted.png", srcCopy);

        // Обнаружение масштаба
        cv::Vec4i scaleLine = detectScaleLine(src);
        if (scaleLine[0] != 0 || scaleLine[1] != 0 || scaleLine[2] != 0 || scaleLine[3] != 0) {
            std::string scaleText = detectScaleText(src, scaleLine);
            float scale = calculateScale(scaleLine, scaleText);
            for (auto& cell : cells) {
                cell.diameterNm = cell.diameterPx * scale;
            }
        }
    }
}

cv::Vec4i ImageProcessor::detectScaleLine(const cv::Mat& image) {
    cv::Mat gray;
    if (image.channels() == 3) {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = image.clone();
    }
    
    // Применяем детектор границ Canny
    cv::Mat edges;
    cv::Canny(gray, edges, 50, 150, 3);
    
    // Используем преобразование Хафа для поиска линий
    std::vector<cv::Vec4i> lines;
    cv::HoughLinesP(edges, lines, 1, CV_PI/180, 50, 50, 10);
    
    // Ищем наиболее горизонтальную линию подходящей длины
    cv::Vec4i bestLine(0, 0, 0, 0);
    double maxLength = 0;
    
    for (const auto& line : lines) {
        double dx = line[2] - line[0];
        double dy = line[3] - line[1];
        double length = std::sqrt(dx*dx + dy*dy);
        double angle = std::abs(std::atan2(dy, dx) * 180 / CV_PI);
        
        // Ищем горизонтальные линии (угол близок к 0 или 180 градусов)
        if ((angle < 10 || angle > 170) && length > maxLength && length > 50) {
            maxLength = length;
            bestLine = line;
        }
    }
    
    return bestLine;
}

float ImageProcessor::calculateScale(const cv::Vec4i& line, const std::string& text) {
    // Расчет длины линии в пикселях
    float lengthPx = std::sqrt(std::pow(line[2] - line[0], 2) + std::pow(line[3] - line[1], 2));
    
    // Парсинг текста для извлечения размера в нанометрах
    // Предполагаем формат типа "100 nm" или "500 нм"
    float scaleNm = 100.0f; // значение по умолчанию
    
    // Простой парсер для извлечения числа из текста
    std::string numStr;
    for (char c : text) {
        if (std::isdigit(c) || c == '.') {
            numStr += c;
        } else if (!numStr.empty()) {
            break;
        }
    }
    
    if (!numStr.empty()) {
        try {
            scaleNm = std::stof(numStr);
        } catch (...) {
            // В случае ошибки используем значение по умолчанию
        }
    }
    
    return scaleNm / lengthPx; // нанометры на пиксель
}

std::string ImageProcessor::detectScaleText(const cv::Mat& image, const cv::Vec4i& line) {
    // Простая реализация: возвращаем стандартное значение
    // В реальном приложении здесь можно использовать OCR (например, Tesseract)
    // для распознавания текста рядом с линией
    
    // Определяем область рядом с линией для поиска текста
    int x1 = std::min(line[0], line[2]);
    int y1 = std::min(line[1], line[3]) - 50; // Область выше линии
    int width = std::abs(line[2] - line[0]) + 100;
    int height = 100;
    
    // Проверяем границы
    x1 = std::max(0, x1 - 50);
    y1 = std::max(0, y1);
    width = std::min(width, image.cols - x1);
    height = std::min(height, image.rows - y1);
    
    // В реальной реализации здесь был бы код OCR
    // Пока возвращаем значение по умолчанию
    return "100 nm";
}
