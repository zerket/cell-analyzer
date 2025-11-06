#ifndef NEURALNETPARAMETERSWIDGET_H
#define NEURALNETPARAMETERSWIDGET_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QMap>
#include "neuralnetdetector.h"

/**
 * @brief Виджет для настройки параметров нейросетевой детекции
 *
 * Позволяет:
 * - Выбрать ONNX модель
 * - Настроить параметры детекции (confidence, размеры клеток, NMS)
 * - Определить классы клеток и их названия
 * - Выбрать использование GPU
 */
class NeuralNetParametersWidget : public QWidget {
    Q_OBJECT

public:
    explicit NeuralNetParametersWidget(QWidget *parent = nullptr);
    ~NeuralNetParametersWidget();

    /**
     * @brief Получить текущие параметры детекции
     */
    NeuralNetDetector::NeuralNetParams getParameters() const;

    /**
     * @brief Установить параметры детекции
     */
    void setParameters(const NeuralNetDetector::NeuralNetParams& params);

    /**
     * @brief Проверить валидность параметров (загружена ли модель)
     */
    bool isValid() const;

    /**
     * @brief Получить путь к модели
     */
    QString getModelPath() const;

signals:
    /**
     * @brief Сигнал об изменении параметров
     */
    void parametersChanged();

    /**
     * @brief Сигнал о загрузке модели
     */
    void modelLoaded(bool success);

private slots:
    void onBrowseModel();
    void onLoadModel();
    void onParameterChanged();
    void onNumClassesChanged(int numClasses);
    void onAddClassMapping();
    void onRemoveClassMapping();
    void onTestGPU();

private:
    void setupUI();
    void createModelSelectionGroup();
    void createDetectionParamsGroup();
    void createClassMappingGroup();
    void createPostprocessingGroup();
    void createGPUSettingsGroup();

    void updateClassMappingTable();
    void loadClassMappingsFromUI();

    QWidget* createLabeledControl(const QString& label, QWidget* control,
                                   const QString& tooltip = "");

private:
    // Группы параметров
    QGroupBox* modelGroup;
    QGroupBox* detectionGroup;
    QGroupBox* classMappingGroup;
    QGroupBox* postprocessGroup;
    QGroupBox* gpuGroup;

    // Выбор модели
    QLineEdit* modelPathEdit;
    QPushButton* browseButton;
    QPushButton* loadModelButton;
    QLabel* modelStatusLabel;

    // Параметры детекции
    QSpinBox* inputSizeSpin;
    QDoubleSpinBox* confidenceSpin;
    QSpinBox* minCellSizeSpin;
    QSpinBox* maxCellSizeSpin;
    QDoubleSpinBox* nmsThresholdSpin;

    // Классификация
    QSpinBox* numClassesSpin;
    QWidget* classMappingWidget;
    QVBoxLayout* classMappingLayout;
    QMap<int, QLineEdit*> classNameEdits; // classId -> QLineEdit

    // Постобработка
    QCheckBox* fillHolesCheck;
    QSpinBox* morphKernelSizeSpin;
    QDoubleSpinBox* minCircularitySpin;
    QDoubleSpinBox* maxCircularitySpin;

    // GPU настройки
    QCheckBox* useGPUCheck;
    QComboBox* gpuDeviceCombo;
    QPushButton* testGPUButton;
    QLabel* gpuStatusLabel;

    // Детектор для загрузки и тестирования модели
    NeuralNetDetector* detector;

    // Текущие параметры
    NeuralNetDetector::NeuralNetParams m_currentParams;
    bool m_modelValid;
};

#endif // NEURALNETPARAMETERSWIDGET_H
