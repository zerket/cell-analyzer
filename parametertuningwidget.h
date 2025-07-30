#ifndef PARAMETERTUNINGWIDGET_H
#define PARAMETERTUNINGWIDGET_H

#include <QWidget>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <opencv2/opencv.hpp>
#include "imageprocessor.h"

class ParameterTuningWidget : public QWidget {
    Q_OBJECT

public:
    using HoughParams = ImageProcessor::HoughParams;

    explicit ParameterTuningWidget(const QString& imagePath, QWidget *parent = nullptr);

signals:
    void parametersConfirmed(const HoughParams& params);

private slots:
    void onParameterChanged();
    void onConfirmClicked();
    void onSavePreset();
    void onLoadPreset();

private:
    void setupUI();
    void updatePreview();
    void loadPresets();
    void savePresets();

    QString m_imagePath;
    cv::Mat m_originalImage;
    cv::Mat m_grayImage;
    cv::Mat m_blurredImage;
    
    QLabel* m_previewLabel;
    QDoubleSpinBox* m_dpSpinBox;
    QDoubleSpinBox* m_minDistSpinBox;
    QDoubleSpinBox* m_param1SpinBox;
    QDoubleSpinBox* m_param2SpinBox;
    QSpinBox* m_minRadiusSpinBox;
    QSpinBox* m_maxRadiusSpinBox;
    QPushButton* m_confirmButton;
    QComboBox* m_presetCombo;
    
    HoughParams m_currentParams;
    QMap<QString, HoughParams> m_presets;
};

#endif // PARAMETERTUNINGWIDGET_H