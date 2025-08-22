#ifndef PARAMETERTUNINGWIDGET_H
#define PARAMETERTUNINGWIDGET_H

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMouseEvent>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include "imageprocessor.h"

class InteractiveImageLabel : public QLabel {
    Q_OBJECT

public:
    explicit InteractiveImageLabel(QWidget* parent = nullptr) : QLabel(parent) {}
    
    void setImageSize(const QSize& size) { m_imageSize = size; }
    QSize getImageSize() const { return m_imageSize; }
    
signals:
    void imageClicked(QPoint position);
    
protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && !pixmap().isNull()) {
            // Правильно вычисляем координаты с учетом выравнивания изображения
            QPoint clickPos = getImageCoordinate(event->pos());
            if (clickPos.x() >= 0 && clickPos.y() >= 0) { // Проверяем, что клик по изображению
                emit imageClicked(clickPos);
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
    void onParameterChangedDelayed();
    void onApplyParameters();
    void onConfirmClicked();
    void onSavePreset();
    void onLoadPreset();
    void onImageClicked(QPoint position);
    void onClearSelection();
    void onAutoFitParameters();
    void onResetAll();
    void onDeletePreset();

private:
    void setupUI();
    void updatePreview();
    void showOriginalImage();
    void loadPresets();
    void savePresets();
    void drawSelectedCells(cv::Mat& image);
    void optimizeParametersForSelectedCells();
    HoughParams findBestParametersForCells(const std::vector<cv::Point>& selectedCells);
    double evaluateParametersForCells(const HoughParams& params, const std::vector<cv::Point>& selectedCells);
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
    
    std::vector<cv::Point> m_selectedCells;
    QSize m_scaledImageSize;
    double m_scaleFactorX;
    double m_scaleFactorY;
    bool m_parametersApplied;
    
    QTimer* m_parameterUpdateTimer;
};

#endif // PARAMETERTUNINGWIDGET_H