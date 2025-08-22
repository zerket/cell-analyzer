#include "algorithmselectionwidget.h"
#include "logger.h"
#include <QScrollArea>
#include <QSplitter>
#include <QToolTip>

AlgorithmSelectionWidget::AlgorithmSelectionWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    
    // Инициализируем параметры по умолчанию
    m_currentParams = AdvancedDetector::DetectionParams();
    setDetectionParams(m_currentParams);
}

AlgorithmSelectionWidget::~AlgorithmSelectionWidget() {
}

void AlgorithmSelectionWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Заголовок
    QLabel* titleLabel = new QLabel("Настройка алгоритма обнаружения");
    titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; margin: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    setupAlgorithmSelection();
    setupParameterPanels();
    
    // Кнопки управления
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    
    resetButton = new QPushButton("🔄 Сбросить к умолчанию");
    resetButton->setStyleSheet("QPushButton { background-color: #FF9800; color: white; border-radius: 8px; padding: 8px 16px; }");
    connect(resetButton, &QPushButton::clicked, this, &AlgorithmSelectionWidget::resetToDefaults);
    buttonLayout->addWidget(resetButton);
    
    buttonLayout->addStretch();
    
    presetsButton = new QPushButton("💾 Пресеты");
    presetsButton->setStyleSheet("QPushButton { background-color: #607D8B; color: white; border-radius: 8px; padding: 8px 16px; }");
    buttonLayout->addWidget(presetsButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // Добавляем основные элементы
    mainLayout->addWidget(algorithmCombo);
    mainLayout->addWidget(descriptionLabel);
    mainLayout->addWidget(parameterStack, 1);
    
    setLayout(mainLayout);
}

void AlgorithmSelectionWidget::setupAlgorithmSelection() {
    // Выбор алгоритма
    QGroupBox* algorithmGroup = new QGroupBox("Алгоритм обнаружения");
    QVBoxLayout* algorithmLayout = new QVBoxLayout(algorithmGroup);
    
    algorithmCombo = new QComboBox();
    algorithmCombo->addItem("🔴 Преобразование Хафа (круги)", static_cast<int>(AdvancedDetector::DetectionAlgorithm::HoughCircles));
    algorithmCombo->addItem("📐 Обнаружение контуров", static_cast<int>(AdvancedDetector::DetectionAlgorithm::ContourBased));
    algorithmCombo->addItem("💧 Водораздельная сегментация", static_cast<int>(AdvancedDetector::DetectionAlgorithm::WatershedSegmentation));
    algorithmCombo->addItem("🔀 Морфологические операции", static_cast<int>(AdvancedDetector::DetectionAlgorithm::MorphologicalOperations));
    algorithmCombo->addItem("⚡ Адаптивное пороговое значение", static_cast<int>(AdvancedDetector::DetectionAlgorithm::AdaptiveThreshold));
    algorithmCombo->addItem("🎯 Детектор блобов", static_cast<int>(AdvancedDetector::DetectionAlgorithm::BlobDetection));
    
    algorithmCombo->setStyleSheet("QComboBox { padding: 8px; border: 2px solid #ddd; border-radius: 8px; }");
    connect(algorithmCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AlgorithmSelectionWidget::onAlgorithmChanged);
    
    algorithmLayout->addWidget(algorithmCombo);
    
    // Описание алгоритма
    descriptionLabel = createDescriptionLabel("Выберите алгоритм обнаружения");
    algorithmLayout->addWidget(descriptionLabel);
}

void AlgorithmSelectionWidget::setupParameterPanels() {
    parameterStack = new QStackedWidget();
    
    setupGeneralParams();
    setupContourParams(); 
    setupWatershedParams();
    setupMorphologyParams();
    setupAdaptiveParams();
    setupBlobParams();
    
    // Добавляем панели в стек
    parameterStack->addWidget(generalParamsPanel);
    parameterStack->addWidget(contourParamsPanel);
    parameterStack->addWidget(watershedParamsPanel);
    parameterStack->addWidget(morphologyParamsPanel);
    parameterStack->addWidget(adaptiveParamsPanel);
    parameterStack->addWidget(blobParamsPanel);
}

void AlgorithmSelectionWidget::setupGeneralParams() {
    generalParamsPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(generalParamsPanel);
    
    QGroupBox* sizeGroup = new QGroupBox("Размеры клеток");
    QGridLayout* sizeLayout = new QGridLayout(sizeGroup);
    
    // Минимальная площадь
    minCellAreaSpin = new QSpinBox();
    minCellAreaSpin->setRange(50, 50000);
    minCellAreaSpin->setValue(500);
    minCellAreaSpin->setSuffix(" пикс²");
    connect(minCellAreaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    sizeLayout->addWidget(createParameterRow("Мин. площадь:", minCellAreaSpin, "Минимальная площадь клетки в пикселях"), 0, 0);
    
    // Максимальная площадь
    maxCellAreaSpin = new QSpinBox();
    maxCellAreaSpin->setRange(500, 100000);
    maxCellAreaSpin->setValue(15000);
    maxCellAreaSpin->setSuffix(" пикс²");
    connect(maxCellAreaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    sizeLayout->addWidget(createParameterRow("Макс. площадь:", maxCellAreaSpin, "Максимальная площадь клетки в пикселях"), 1, 0);
    
    layout->addWidget(sizeGroup);
    
    QGroupBox* shapeGroup = new QGroupBox("Форма клеток");
    QGridLayout* shapeLayout = new QGridLayout(shapeGroup);
    
    // Минимальная круглость
    minCircularitySpin = new QDoubleSpinBox();
    minCircularitySpin->setRange(0.1, 1.0);
    minCircularitySpin->setValue(0.3);
    minCircularitySpin->setDecimals(2);
    minCircularitySpin->setSingleStep(0.05);
    connect(minCircularitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    shapeLayout->addWidget(createParameterRow("Мин. круглость:", minCircularitySpin, "Минимальная круглость (0.1 - любая форма, 1.0 - идеальный круг)"), 0, 0);
    
    // Максимальная круглость
    maxCircularitySpin = new QDoubleSpinBox();
    maxCircularitySpin->setRange(0.1, 1.0);
    maxCircularitySpin->setValue(1.0);
    maxCircularitySpin->setDecimals(2);
    maxCircularitySpin->setSingleStep(0.05);
    connect(maxCircularitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    shapeLayout->addWidget(createParameterRow("Макс. круглость:", maxCircularitySpin, "Максимальная круглость"), 1, 0);
    
    layout->addWidget(shapeGroup);
    layout->addStretch();
}

void AlgorithmSelectionWidget::setupContourParams() {
    contourParamsPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(contourParamsPanel);
    
    QGroupBox* contourGroup = new QGroupBox("Параметры контуров");
    QGridLayout* contourLayout = new QGridLayout(contourGroup);
    
    // Минимальный периметр
    contourMinPerimeterSpin = new QDoubleSpinBox();
    contourMinPerimeterSpin->setRange(10.0, 1000.0);
    contourMinPerimeterSpin->setValue(50.0);
    contourMinPerimeterSpin->setSuffix(" пикс");
    connect(contourMinPerimeterSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    contourLayout->addWidget(createParameterRow("Мин. периметр:", contourMinPerimeterSpin), 0, 0);
    
    // Максимальный периметр
    contourMaxPerimeterSpin = new QDoubleSpinBox();
    contourMaxPerimeterSpin->setRange(100.0, 2000.0);
    contourMaxPerimeterSpin->setValue(800.0);
    contourMaxPerimeterSpin->setSuffix(" пикс");
    connect(contourMaxPerimeterSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    contourLayout->addWidget(createParameterRow("Макс. периметр:", contourMaxPerimeterSpin), 1, 0);
    
    // Точность аппроксимации
    contourApproxEpsilonSpin = new QDoubleSpinBox();
    contourApproxEpsilonSpin->setRange(0.005, 0.1);
    contourApproxEpsilonSpin->setValue(0.02);
    contourApproxEpsilonSpin->setDecimals(3);
    contourApproxEpsilonSpin->setSingleStep(0.005);
    connect(contourApproxEpsilonSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    contourLayout->addWidget(createParameterRow("Точность аппрокс.:", contourApproxEpsilonSpin, "Точность аппроксимации контура"), 2, 0);
    
    layout->addWidget(contourGroup);
    layout->addStretch();
}

void AlgorithmSelectionWidget::setupWatershedParams() {
    watershedParamsPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(watershedParamsPanel);
    
    QGroupBox* watershedGroup = new QGroupBox("Параметры водораздела");
    QGridLayout* watershedLayout = new QGridLayout(watershedGroup);
    
    // Количество маркеров
    watershedMarkersSpin = new QSpinBox();
    watershedMarkersSpin->setRange(0, 1000);
    watershedMarkersSpin->setValue(0);
    watershedMarkersSpin->setSpecialValueText("Автоматически");
    connect(watershedMarkersSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    watershedLayout->addWidget(createParameterRow("Маркеры:", watershedMarkersSpin, "0 - автоматическое определение"), 0, 0);
    
    // Минимальное расстояние
    watershedMinDistanceSpin = new QDoubleSpinBox();
    watershedMinDistanceSpin->setRange(5.0, 100.0);
    watershedMinDistanceSpin->setValue(20.0);
    watershedMinDistanceSpin->setSuffix(" пикс");
    connect(watershedMinDistanceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    watershedLayout->addWidget(createParameterRow("Мин. расстояние:", watershedMinDistanceSpin), 1, 0);
    
    layout->addWidget(watershedGroup);
    layout->addStretch();
}

void AlgorithmSelectionWidget::setupMorphologyParams() {
    morphologyParamsPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(morphologyParamsPanel);
    
    QGroupBox* morphGroup = new QGroupBox("Морфологические параметры");
    QGridLayout* morphLayout = new QGridLayout(morphGroup);
    
    // Размер ядра
    morphKernelSizeSpin = new QSpinBox();
    morphKernelSizeSpin->setRange(3, 21);
    morphKernelSizeSpin->setValue(5);
    morphKernelSizeSpin->setSingleStep(2); // Только нечетные значения
    connect(morphKernelSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    morphLayout->addWidget(createParameterRow("Размер ядра:", morphKernelSizeSpin), 0, 0);
    
    // Количество итераций
    morphIterationsSpin = new QSpinBox();
    morphIterationsSpin->setRange(1, 10);
    morphIterationsSpin->setValue(2);
    connect(morphIterationsSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    morphLayout->addWidget(createParameterRow("Итерации:", morphIterationsSpin), 1, 0);
    
    // Форма ядра
    morphShapeCombo = new QComboBox();
    morphShapeCombo->addItem("Прямоугольник", static_cast<int>(cv::MORPH_RECT));
    morphShapeCombo->addItem("Эллипс", static_cast<int>(cv::MORPH_ELLIPSE));
    morphShapeCombo->addItem("Крест", static_cast<int>(cv::MORPH_CROSS));
    morphShapeCombo->setCurrentIndex(1); // По умолчанию эллипс
    connect(morphShapeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    morphLayout->addWidget(createParameterRow("Форма ядра:", morphShapeCombo), 2, 0);
    
    layout->addWidget(morphGroup);
    layout->addStretch();
}

void AlgorithmSelectionWidget::setupAdaptiveParams() {
    adaptiveParamsPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(adaptiveParamsPanel);
    
    QGroupBox* adaptiveGroup = new QGroupBox("Адаптивное пороговое значение");
    QGridLayout* adaptiveLayout = new QGridLayout(adaptiveGroup);
    
    // Размер блока
    adaptiveBlockSizeSpin = new QSpinBox();
    adaptiveBlockSizeSpin->setRange(3, 31);
    adaptiveBlockSizeSpin->setValue(11);
    adaptiveBlockSizeSpin->setSingleStep(2); // Только нечетные значения
    connect(adaptiveBlockSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    adaptiveLayout->addWidget(createParameterRow("Размер блока:", adaptiveBlockSizeSpin), 0, 0);
    
    // Константа C
    adaptiveCSpina = new QDoubleSpinBox();
    adaptiveCSpina->setRange(-10.0, 10.0);
    adaptiveCSpina->setValue(2.0);
    adaptiveCSpina->setSingleStep(0.5);
    connect(adaptiveCSpina, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    adaptiveLayout->addWidget(createParameterRow("Константа C:", adaptiveCSpina), 1, 0);
    
    // Метод
    adaptiveMethodCombo = new QComboBox();
    adaptiveMethodCombo->addItem("Среднее", static_cast<int>(cv::ADAPTIVE_THRESH_MEAN_C));
    adaptiveMethodCombo->addItem("Гауссово", static_cast<int>(cv::ADAPTIVE_THRESH_GAUSSIAN_C));
    adaptiveMethodCombo->setCurrentIndex(1); // По умолчанию Гауссово
    connect(adaptiveMethodCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    adaptiveLayout->addWidget(createParameterRow("Метод:", adaptiveMethodCombo), 2, 0);
    
    layout->addWidget(adaptiveGroup);
    layout->addStretch();
}

void AlgorithmSelectionWidget::setupBlobParams() {
    blobParamsPanel = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(blobParamsPanel);
    
    QGroupBox* blobGroup = new QGroupBox("Параметры детектора блобов");
    QGridLayout* blobLayout = new QGridLayout(blobGroup);
    
    // Минимальный порог
    blobMinThresholdSpin = new QDoubleSpinBox();
    blobMinThresholdSpin->setRange(10.0, 200.0);
    blobMinThresholdSpin->setValue(50.0);
    connect(blobMinThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    blobLayout->addWidget(createParameterRow("Мин. порог:", blobMinThresholdSpin), 0, 0);
    
    // Максимальный порог
    blobMaxThresholdSpin = new QDoubleSpinBox();
    blobMaxThresholdSpin->setRange(100.0, 255.0);
    blobMaxThresholdSpin->setValue(220.0);
    connect(blobMaxThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    blobLayout->addWidget(createParameterRow("Макс. порог:", blobMaxThresholdSpin), 1, 0);
    
    // Шаг порога
    blobThresholdStepSpin = new QDoubleSpinBox();
    blobThresholdStepSpin->setRange(1.0, 50.0);
    blobThresholdStepSpin->setValue(10.0);
    connect(blobThresholdStepSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    blobLayout->addWidget(createParameterRow("Шаг порога:", blobThresholdStepSpin), 2, 0);
    
    // Минимальная повторяемость
    blobMinRepeatabilitySpina = new QSpinBox();
    blobMinRepeatabilitySpina->setRange(1, 10);
    blobMinRepeatabilitySpina->setValue(2);
    connect(blobMinRepeatabilitySpina, QOverload<int>::of(&QSpinBox::valueChanged), this, &AlgorithmSelectionWidget::onParameterChanged);
    blobLayout->addWidget(createParameterRow("Мин. повторяемость:", blobMinRepeatabilitySpina, "Минимальное количество уровней порога, на которых должен обнаруживаться блоб"), 3, 0);
    
    layout->addWidget(blobGroup);
    layout->addStretch();
}

QWidget* AlgorithmSelectionWidget::createParameterRow(const QString& label, QWidget* control, const QString& tooltip) {
    QWidget* row = new QWidget();
    QHBoxLayout* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    
    QLabel* labelWidget = new QLabel(label);
    labelWidget->setMinimumWidth(120);
    
    if (!tooltip.isEmpty()) {
        labelWidget->setToolTip(tooltip);
        control->setToolTip(tooltip);
    }
    
    layout->addWidget(labelWidget);
    layout->addWidget(control);
    layout->addStretch();
    
    return row;
}

QLabel* AlgorithmSelectionWidget::createDescriptionLabel(const QString& text) {
    QLabel* label = new QLabel(text);
    label->setWordWrap(true);
    label->setStyleSheet("QLabel { background-color: #f0f8ff; border: 1px solid #ddd; border-radius: 8px; padding: 10px; margin: 5px; }");
    return label;
}

void AlgorithmSelectionWidget::onAlgorithmChanged(int index) {
    AdvancedDetector::DetectionAlgorithm algorithm = static_cast<AdvancedDetector::DetectionAlgorithm>(algorithmCombo->itemData(index).toInt());
    
    // Обновляем описание
    QString description = AdvancedDetector::getAlgorithmDescription(algorithm);
    switch (algorithm) {
        case AdvancedDetector::DetectionAlgorithm::HoughCircles:
            description += "\n\nОптимально для: круглых клеток, четких границ";
            break;
        case AdvancedDetector::DetectionAlgorithm::ContourBased:
            description += "\n\nОптимально для: клеток произвольной формы, четких контуров";
            break;
        case AdvancedDetector::DetectionAlgorithm::WatershedSegmentation:
            description += "\n\nОптимально для: перекрывающихся клеток, сложных текстур";
            break;
        case AdvancedDetector::DetectionAlgorithm::MorphologicalOperations:
            description += "\n\nОптимально для: зашумленных изображений, размытых границ";
            break;
        case AdvancedDetector::DetectionAlgorithm::AdaptiveThreshold:
            description += "\n\nОптимально для: изображений с неравномерным освещением";
            break;
        case AdvancedDetector::DetectionAlgorithm::BlobDetection:
            description += "\n\nОптимально для: компактных объектов, высокого контраста";
            break;
    }
    
    descriptionLabel->setText(description);
    
    // Обновляем панель параметров
    updateParameterPanel();
    
    // Обновляем текущие параметры
    m_currentParams.algorithm = algorithm;
    
    emit algorithmChanged(algorithm);
    emit parametersChanged();
}

void AlgorithmSelectionWidget::updateParameterPanel() {
    AdvancedDetector::DetectionAlgorithm algorithm = getCurrentAlgorithm();
    
    switch (algorithm) {
        case AdvancedDetector::DetectionAlgorithm::HoughCircles:
            parameterStack->setCurrentWidget(generalParamsPanel);
            break;
        case AdvancedDetector::DetectionAlgorithm::ContourBased:
            parameterStack->setCurrentWidget(contourParamsPanel);
            break;
        case AdvancedDetector::DetectionAlgorithm::WatershedSegmentation:
            parameterStack->setCurrentWidget(watershedParamsPanel);
            break;
        case AdvancedDetector::DetectionAlgorithm::MorphologicalOperations:
            parameterStack->setCurrentWidget(morphologyParamsPanel);
            break;
        case AdvancedDetector::DetectionAlgorithm::AdaptiveThreshold:
            parameterStack->setCurrentWidget(adaptiveParamsPanel);
            break;
        case AdvancedDetector::DetectionAlgorithm::BlobDetection:
            parameterStack->setCurrentWidget(blobParamsPanel);
            break;
    }
}

void AlgorithmSelectionWidget::onParameterChanged() {
    emit parametersChanged();
}

AdvancedDetector::DetectionParams AlgorithmSelectionWidget::getDetectionParams() const {
    AdvancedDetector::DetectionParams params = m_currentParams;
    
    // Обновляем общие параметры
    params.minCellArea = minCellAreaSpin->value();
    params.maxCellArea = maxCellAreaSpin->value();
    params.minCircularity = minCircularitySpin->value();
    params.maxCircularity = maxCircularitySpin->value();
    
    // Обновляем специфичные параметры в зависимости от алгоритма
    params.contourMinPerimeter = contourMinPerimeterSpin->value();
    params.contourMaxPerimeter = contourMaxPerimeterSpin->value();
    params.contourApproxEpsilon = contourApproxEpsilonSpin->value();
    
    params.watershedMarkers = watershedMarkersSpin->value();
    params.watershedMinDistance = watershedMinDistanceSpin->value();
    
    params.morphKernelSize = morphKernelSizeSpin->value();
    params.morphIterations = morphIterationsSpin->value();
    params.morphShape = static_cast<cv::MorphShapes>(morphShapeCombo->currentData().toInt());
    
    params.adaptiveBlockSize = adaptiveBlockSizeSpin->value();
    params.adaptiveC = adaptiveCSpina->value();
    params.adaptiveMethod = static_cast<cv::AdaptiveThresholdTypes>(adaptiveMethodCombo->currentData().toInt());
    
    params.blobMinThreshold = static_cast<float>(blobMinThresholdSpin->value());
    params.blobMaxThreshold = static_cast<float>(blobMaxThresholdSpin->value());
    params.blobThresholdStep = static_cast<float>(blobThresholdStepSpin->value());
    params.blobMinRepeatability = static_cast<size_t>(blobMinRepeatabilitySpina->value());
    
    return params;
}

void AlgorithmSelectionWidget::setDetectionParams(const AdvancedDetector::DetectionParams& params) {
    m_currentParams = params;
    
    // Устанавливаем алгоритм
    for (int i = 0; i < algorithmCombo->count(); i++) {
        if (static_cast<AdvancedDetector::DetectionAlgorithm>(algorithmCombo->itemData(i).toInt()) == params.algorithm) {
            algorithmCombo->setCurrentIndex(i);
            break;
        }
    }
    
    // Устанавливаем общие параметры
    minCellAreaSpin->setValue(params.minCellArea);
    maxCellAreaSpin->setValue(params.maxCellArea);
    minCircularitySpin->setValue(params.minCircularity);
    maxCircularitySpin->setValue(params.maxCircularity);
    
    // Устанавливаем специфичные параметры
    contourMinPerimeterSpin->setValue(params.contourMinPerimeter);
    contourMaxPerimeterSpin->setValue(params.contourMaxPerimeter);
    contourApproxEpsilonSpin->setValue(params.contourApproxEpsilon);
    
    watershedMarkersSpin->setValue(params.watershedMarkers);
    watershedMinDistanceSpin->setValue(params.watershedMinDistance);
    
    morphKernelSizeSpin->setValue(params.morphKernelSize);
    morphIterationsSpin->setValue(params.morphIterations);
    
    for (int i = 0; i < morphShapeCombo->count(); i++) {
        if (static_cast<cv::MorphShapes>(morphShapeCombo->itemData(i).toInt()) == params.morphShape) {
            morphShapeCombo->setCurrentIndex(i);
            break;
        }
    }
    
    adaptiveBlockSizeSpin->setValue(params.adaptiveBlockSize);
    adaptiveCSpina->setValue(params.adaptiveC);
    
    for (int i = 0; i < adaptiveMethodCombo->count(); i++) {
        if (static_cast<cv::AdaptiveThresholdTypes>(adaptiveMethodCombo->itemData(i).toInt()) == params.adaptiveMethod) {
            adaptiveMethodCombo->setCurrentIndex(i);
            break;
        }
    }
    
    blobMinThresholdSpin->setValue(params.blobMinThreshold);
    blobMaxThresholdSpin->setValue(params.blobMaxThreshold);
    blobThresholdStepSpin->setValue(params.blobThresholdStep);
    blobMinRepeatabilitySpina->setValue(static_cast<int>(params.blobMinRepeatability));
}

AdvancedDetector::DetectionAlgorithm AlgorithmSelectionWidget::getCurrentAlgorithm() const {
    return static_cast<AdvancedDetector::DetectionAlgorithm>(algorithmCombo->currentData().toInt());
}

void AlgorithmSelectionWidget::resetToDefaults() {
    AdvancedDetector::DetectionParams defaultParams;
    setDetectionParams(defaultParams);
    Logger::instance().log("Параметры алгоритма сброшены к значениям по умолчанию");
}