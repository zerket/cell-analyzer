#ifndef PARAMETERTUNINGWIDGET_H
#define PARAMETERTUNINGWIDGET_H

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMouseEvent>
#include <QProgressDialog>
#include <opencv2/opencv.hpp>
#include <unordered_map>
#include "imageprocessor.h"

class InteractiveImageLabel : public QLabel {
    Q_OBJECT

public:
    explicit InteractiveImageLabel(QWidget* parent = nullptr) : QLabel(parent) {}
    
    void setImageSize(const QSize& size) { m_imageSize = size; }
    QSize getImageSize() const { return m_imageSize; }
    
signals:
    void imageClicked(QPoint position);
    void imageRightClicked(QPoint position);

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (!pixmap().isNull()) {
            // Правильно вычисляем координаты с учетом выравнивания изображения
            QPoint clickPos = getImageCoordinate(event->pos());
            if (clickPos.x() >= 0 && clickPos.y() >= 0) { // Проверяем, что клик по изображению
                if (event->button() == Qt::LeftButton) {
                    emit imageClicked(clickPos);
                } else if (event->button() == Qt::RightButton) {
                    emit imageRightClicked(clickPos);
                }
            }
        }
        QLabel::mousePressEvent(event);
    }
    
private:
    QPoint getImageCoordinate(const QPoint& labelPos) {
        const QPixmap& pm = pixmap();
        if (pm.isNull()) return QPoint(-1, -1);
        
        // Размеры QLabel и изображения
        QSize labelSize = size();
        QSize pixmapSize = pm.size();
        
        // Вычисляем смещение изображения в QLabel (с учетом выравнивания по центру)
        int xOffset = (labelSize.width() - pixmapSize.width()) / 2;
        int yOffset = (labelSize.height() - pixmapSize.height()) / 2;
        
        // Проверяем, что клик внутри изображения
        QPoint imagePos = labelPos - QPoint(xOffset, yOffset);
        if (imagePos.x() < 0 || imagePos.y() < 0 || 
            imagePos.x() >= pixmapSize.width() || imagePos.y() >= pixmapSize.height()) {
            return QPoint(-1, -1); // Клик вне изображения
        }
        
        return imagePos;
    }
    
private:
    QSize m_imageSize;
};

class ParameterTuningWidget : public QWidget {
    Q_OBJECT

public:
    using HoughParams = ImageProcessor::HoughParams;
    
    struct PresetData {
        HoughParams params;
        double coefficient; // нм/пиксель
        
        PresetData() : coefficient(0.0) {}
        PresetData(const HoughParams& p, double coeff = 0.0) : params(p), coefficient(coeff) {}
    };

    explicit ParameterTuningWidget(const QString& imagePath, QWidget *parent = nullptr);

signals:
    void parametersConfirmed(const HoughParams& params);

private slots:
    void onParameterChanged();
    void onApplyParameters();
    void onConfirmClicked();
    void onSavePreset();
    void onLoadPreset();
    void onImageClicked(QPoint position);
    void onImageRightClicked(QPoint position);
    void onClearSelection();
    void onAutoFitParameters();
    void onResetAll();
    void onDeletePreset();

private:
    // Структура для детальной оценки параметров
    struct EvaluationResult {
        double score;              // Итоговая оценка
        int matchedCells;          // Количество покрытых позитивных маркеров
        int totalCircles;          // Общее количество найденных кругов
        int excessCircles;         // Лишние круги
        int negativeViolations;    // Нарушения негативных маркеров
        double coverageRatio;      // Процент покрытия позитивных маркеров

        EvaluationResult() : score(0.0), matchedCells(0), totalCircles(0),
                           excessCircles(0), negativeViolations(0), coverageRatio(0.0) {}
    };

    // Структура для хеширования параметров (для кэша)
    struct ParamsHash {
        size_t operator()(const HoughParams& p) const {
            // Округляем до разумной точности и комбинируем хеши
            auto h1 = std::hash<int>{}(static_cast<int>(p.dp * 10));
            auto h2 = std::hash<int>{}(static_cast<int>(p.minDist));
            auto h3 = std::hash<int>{}(static_cast<int>(p.param1));
            auto h4 = std::hash<int>{}(static_cast<int>(p.param2));
            auto h5 = std::hash<int>{}(p.minRadius);
            auto h6 = std::hash<int>{}(p.maxRadius);
            return h1 ^ (h2 << 1) ^ (h3 << 2) ^ (h4 << 3) ^ (h5 << 4) ^ (h6 << 5);
        }
    };

    struct ParamsEqual {
        bool operator()(const HoughParams& a, const HoughParams& b) const {
            return static_cast<int>(a.dp * 10) == static_cast<int>(b.dp * 10) &&
                   static_cast<int>(a.minDist) == static_cast<int>(b.minDist) &&
                   static_cast<int>(a.param1) == static_cast<int>(b.param1) &&
                   static_cast<int>(a.param2) == static_cast<int>(b.param2) &&
                   a.minRadius == b.minRadius &&
                   a.maxRadius == b.maxRadius;
        }
    };

    void setupUI();
    void updatePreview();
    void showOriginalImage();
    void loadPresets();
    void savePresets();
    void drawSelectedCells(cv::Mat& image);
    void drawNegativeCells(cv::Mat& image);
    void optimizeParametersForSelectedCells();

    // Новый улучшенный алгоритм оптимизации
    HoughParams findBestParametersForCells(const std::vector<cv::Point>& selectedCells,
                                          const std::vector<cv::Point>& negativeCells,
                                          QProgressDialog* progress);

    // Кэширующая обертка для HoughCircles
    std::vector<cv::Vec3f> detectCirclesWithCache(const HoughParams& params);

    // Новая система оценки с непрерывной штрафной функцией
    EvaluationResult evaluateParametersAdvanced(const HoughParams& params,
                                                const std::vector<cv::Point>& selectedCells,
                                                const std::vector<cv::Point>& negativeCells);

    // Фаза 1: Грубый поиск (большие шаги)
    HoughParams coarsePhaseSearch(const std::vector<cv::Point>& selectedCells,
                                  const std::vector<cv::Point>& negativeCells,
                                  QProgressDialog* progress);

    // Фаза 2: Локальная оптимизация (малые шаги)
    HoughParams finePhaseSearch(const HoughParams& startParams,
                               const std::vector<cv::Point>& selectedCells,
                               const std::vector<cv::Point>& negativeCells,
                               QProgressDialog* progress);

    // Фаза 3: Градиентный спуск
    HoughParams gradientDescent(const HoughParams& startParams,
                               const std::vector<cv::Point>& selectedCells,
                               const std::vector<cv::Point>& negativeCells,
                               QProgressDialog* progress);

    // Проверка эвристик для отсечения плохих комбинаций
    bool isValidHeuristicCombination(double dp, double param1, double param2) const;

    // Старые методы (для совместимости, могут быть удалены)
    double evaluateParametersForCells(const HoughParams& params, const std::vector<cv::Point>& selectedCells, const std::vector<cv::Point>& negativeCells);
    std::pair<double, int> evaluateParametersForCellsWithCount(const HoughParams& params, const std::vector<cv::Point>& selectedCells, const std::vector<cv::Point>& negativeCells);

    cv::Mat loadImageSafely(const QString& imagePath);
    std::vector<cv::Vec3f> filterOverlappingCircles(const std::vector<cv::Vec3f>& circles, double minDist);

    QString m_imagePath;
    cv::Mat m_originalImage;
    cv::Mat m_grayImage;
    cv::Mat m_blurredImage;
    
    InteractiveImageLabel* m_previewLabel;
    QDoubleSpinBox* m_dpSpinBox;
    QDoubleSpinBox* m_minDistSpinBox;
    QDoubleSpinBox* m_param1SpinBox;
    QDoubleSpinBox* m_param2SpinBox;
    QSpinBox* m_minRadiusSpinBox;
    QSpinBox* m_maxRadiusSpinBox;
    QPushButton* m_applyButton;
    QPushButton* m_confirmButton;
    QComboBox* m_presetCombo;
    QPushButton* m_clearSelectionButton;
    QPushButton* m_autoFitButton;
    QLabel* m_selectionInfoLabel;
    
    HoughParams m_currentParams;
    QMap<QString, PresetData> m_presets;

    std::vector<cv::Point> m_selectedCells;      // Позитивные маркеры (ЛКМ) - клетки для обнаружения
    std::vector<cv::Point> m_negativeCells;      // Негативные маркеры (ПКМ) - объекты для исключения
    QSize m_scaledImageSize;
    double m_scaleFactorX;
    double m_scaleFactorY;
    bool m_parametersApplied;

    // Кэш результатов HoughCircles для ускорения повторных вызовов
    std::unordered_map<HoughParams, std::vector<cv::Vec3f>, ParamsHash, ParamsEqual> m_circlesCache;
};

#endif // PARAMETERTUNINGWIDGET_H