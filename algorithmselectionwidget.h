#ifndef ALGORITHMSELECTIONWIDGET_H
#define ALGORITHMSELECTIONWIDGET_H

#include <QWidget>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QStackedWidget>
#include "advanceddetector.h"
#include "neuralnetparameterswidget.h"

class AlgorithmSelectionWidget : public QWidget {
    Q_OBJECT

public:
    explicit AlgorithmSelectionWidget(QWidget *parent = nullptr);
    ~AlgorithmSelectionWidget();
    
    AdvancedDetector::DetectionParams getDetectionParams() const;
    void setDetectionParams(const AdvancedDetector::DetectionParams& params);
    
    AdvancedDetector::DetectionAlgorithm getCurrentAlgorithm() const;

signals:
    void parametersChanged();
    void algorithmChanged(AdvancedDetector::DetectionAlgorithm algorithm);

private slots:
    void onAlgorithmChanged(int index);
    void onParameterChanged();

private:
    void setupUI();
    void setupAlgorithmSelection();
    void setupParameterPanels();
    void setupGeneralParams();
    void setupContourParams();
    void setupWatershedParams();
    void setupMorphologyParams();
    void setupAdaptiveParams();
    void setupBlobParams();
    void setupNeuralNetParams();
    
    void updateParameterPanel();
    void resetToDefaults();
    
    QWidget* createParameterRow(const QString& label, QWidget* control, const QString& tooltip = "");
    QLabel* createDescriptionLabel(const QString& text);
    
private:
    // Main layout elements
    QComboBox* algorithmCombo;
    QLabel* descriptionLabel;
    QStackedWidget* parameterStack;
    QPushButton* resetButton;
    QPushButton* presetsButton;
    
    // Parameter panels
    QWidget* generalParamsPanel;
    QWidget* contourParamsPanel;
    QWidget* watershedParamsPanel;
    QWidget* morphologyParamsPanel;
    QWidget* adaptiveParamsPanel;
    QWidget* blobParamsPanel;
    NeuralNetParametersWidget* neuralNetParamsPanel;
    
    // General parameters
    QSpinBox* minCellAreaSpin;
    QSpinBox* maxCellAreaSpin;
    QDoubleSpinBox* minCircularitySpin;
    QDoubleSpinBox* maxCircularitySpin;
    
    // Contour-based parameters
    QDoubleSpinBox* contourMinPerimeterSpin;
    QDoubleSpinBox* contourMaxPerimeterSpin;
    QDoubleSpinBox* contourApproxEpsilonSpin;
    
    // Watershed parameters
    QSpinBox* watershedMarkersSpin;
    QDoubleSpinBox* watershedMinDistanceSpin;
    
    // Morphological parameters
    QSpinBox* morphKernelSizeSpin;
    QSpinBox* morphIterationsSpin;
    QComboBox* morphShapeCombo;
    
    // Adaptive threshold parameters
    QSpinBox* adaptiveBlockSizeSpin;
    QDoubleSpinBox* adaptiveCSpina;
    QComboBox* adaptiveMethodCombo;
    
    // Blob detection parameters
    QDoubleSpinBox* blobMinThresholdSpin;
    QDoubleSpinBox* blobMaxThresholdSpin;
    QDoubleSpinBox* blobThresholdStepSpin;
    QSpinBox* blobMinRepeatabilitySpina;
    
    // Current settings
    AdvancedDetector::DetectionParams m_currentParams;
};

#endif // ALGORITHMSELECTIONWIDGET_H