#include "neuralnetparameterswidget.h"
#include "thememanager.h"
#include "logger.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFormLayout>
#include <QScrollArea>

NeuralNetParametersWidget::NeuralNetParametersWidget(QWidget *parent)
    : QWidget(parent)
    , detector(new NeuralNetDetector())
    , m_modelValid(false)
{
    setupUI();

    // Инициализация параметров по умолчанию
    m_currentParams.inputSize = 512;
    m_currentParams.confidenceThreshold = 0.5f;
    m_currentParams.minCellSize = 50;
    m_currentParams.maxCellSize = 1000;
    m_currentParams.nmsThreshold = 0.3f;
    m_currentParams.useGPU = false;
    m_currentParams.numClasses = 3;
    m_currentParams.fillHoles = true;
    m_currentParams.morphKernelSize = 3;
    m_currentParams.minCircularity = 0.0;
    m_currentParams.maxCircularity = 1.0;

    // Дефолтные названия классов
    m_currentParams.classNames[1] = "Type A";
    m_currentParams.classNames[2] = "Type B";
    m_currentParams.classNames[3] = "Type C";

    setParameters(m_currentParams);
}

NeuralNetParametersWidget::~NeuralNetParametersWidget() {
    delete detector;
}

void NeuralNetParametersWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);

    // Создание scroll area для длинного списка параметров
    QScrollArea* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    QWidget* scrollContent = new QWidget();
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollContent);

    createModelSelectionGroup();
    createDetectionParamsGroup();
    createClassMappingGroup();
    createPostprocessingGroup();
    createGPUSettingsGroup();

    scrollLayout->addWidget(modelGroup);
    scrollLayout->addWidget(detectionGroup);
    scrollLayout->addWidget(classMappingGroup);
    scrollLayout->addWidget(postprocessGroup);
    scrollLayout->addWidget(gpuGroup);
    scrollLayout->addStretch();

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea);

    // Применение темы
    ThemeManager::instance().applyTheme(this);
}

void NeuralNetParametersWidget::createModelSelectionGroup() {
    modelGroup = new QGroupBox("Модель (ONNX)");
    QVBoxLayout* layout = new QVBoxLayout(modelGroup);

    // Выбор файла модели
    QHBoxLayout* modelPathLayout = new QHBoxLayout();
    modelPathEdit = new QLineEdit();
    modelPathEdit->setPlaceholderText("Путь к .onnx файлу...");
    modelPathEdit->setReadOnly(false);

    browseButton = new QPushButton("Обзор...");
    browseButton->setMaximumWidth(100);
    connect(browseButton, &QPushButton::clicked, this, &NeuralNetParametersWidget::onBrowseModel);

    modelPathLayout->addWidget(new QLabel("Модель:"));
    modelPathLayout->addWidget(modelPathEdit, 1);
    modelPathLayout->addWidget(browseButton);

    layout->addLayout(modelPathLayout);

    // Кнопка загрузки модели
    loadModelButton = new QPushButton("🔄 Загрузить модель");
    connect(loadModelButton, &QPushButton::clicked, this, &NeuralNetParametersWidget::onLoadModel);
    layout->addWidget(loadModelButton);

    // Статус модели
    modelStatusLabel = new QLabel("Модель не загружена");
    modelStatusLabel->setStyleSheet("color: #f44336; font-weight: bold;");
    layout->addWidget(modelStatusLabel);
}

void NeuralNetParametersWidget::createDetectionParamsGroup() {
    detectionGroup = new QGroupBox("Параметры детекции");
    QGridLayout* layout = new QGridLayout(detectionGroup);

    int row = 0;

    // Input size
    inputSizeSpin = new QSpinBox();
    inputSizeSpin->setRange(256, 2048);
    inputSizeSpin->setSingleStep(64);
    inputSizeSpin->setValue(512);
    inputSizeSpin->setToolTip("Размер входного изображения для сети (обычно 512 или 1024)");
    connect(inputSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("Размер входа:"), row, 0);
    layout->addWidget(inputSizeSpin, row, 1);
    layout->addWidget(new QLabel("пикселей"), row, 2);
    row++;

    // Confidence threshold
    confidenceSpin = new QDoubleSpinBox();
    confidenceSpin->setRange(0.0, 1.0);
    confidenceSpin->setSingleStep(0.05);
    confidenceSpin->setDecimals(2);
    confidenceSpin->setValue(0.5);
    confidenceSpin->setToolTip("Порог уверенности детекции (0.0 - 1.0)");
    connect(confidenceSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("Порог уверенности:"), row, 0);
    layout->addWidget(confidenceSpin, row, 1);
    row++;

    // Min cell size
    minCellSizeSpin = new QSpinBox();
    minCellSizeSpin->setRange(10, 10000);
    minCellSizeSpin->setValue(50);
    minCellSizeSpin->setToolTip("Минимальный размер клетки в пикселях");
    connect(minCellSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("Мин. размер клетки:"), row, 0);
    layout->addWidget(minCellSizeSpin, row, 1);
    layout->addWidget(new QLabel("пикселей"), row, 2);
    row++;

    // Max cell size
    maxCellSizeSpin = new QSpinBox();
    maxCellSizeSpin->setRange(10, 50000);
    maxCellSizeSpin->setValue(1000);
    maxCellSizeSpin->setToolTip("Максимальный размер клетки в пикселях");
    connect(maxCellSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("Макс. размер клетки:"), row, 0);
    layout->addWidget(maxCellSizeSpin, row, 1);
    layout->addWidget(new QLabel("пикселей"), row, 2);
    row++;

    // NMS threshold
    nmsThresholdSpin = new QDoubleSpinBox();
    nmsThresholdSpin->setRange(0.0, 1.0);
    nmsThresholdSpin->setSingleStep(0.05);
    nmsThresholdSpin->setDecimals(2);
    nmsThresholdSpin->setValue(0.3);
    nmsThresholdSpin->setToolTip("IoU порог для Non-Maximum Suppression (удаление дубликатов)");
    connect(nmsThresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("NMS порог:"), row, 0);
    layout->addWidget(nmsThresholdSpin, row, 1);
    row++;
}

void NeuralNetParametersWidget::createClassMappingGroup() {
    classMappingGroup = new QGroupBox("Классы клеток");
    QVBoxLayout* layout = new QVBoxLayout(classMappingGroup);

    // Количество классов
    QHBoxLayout* numClassesLayout = new QHBoxLayout();
    numClassesLayout->addWidget(new QLabel("Количество типов клеток:"));
    numClassesSpin = new QSpinBox();
    numClassesSpin->setRange(1, 10);
    numClassesSpin->setValue(3);
    numClassesSpin->setToolTip("Количество различных типов клеток (без учета фона)");
    connect(numClassesSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onNumClassesChanged);
    numClassesLayout->addWidget(numClassesSpin);
    numClassesLayout->addStretch();
    layout->addLayout(numClassesLayout);

    // Виджет для маппинга классов
    classMappingWidget = new QWidget();
    classMappingLayout = new QVBoxLayout(classMappingWidget);
    classMappingLayout->setSpacing(5);
    layout->addWidget(classMappingWidget);

    // Инициализация таблицы классов
    updateClassMappingTable();
}

void NeuralNetParametersWidget::createPostprocessingGroup() {
    postprocessGroup = new QGroupBox("Постобработка");
    QGridLayout* layout = new QGridLayout(postprocessGroup);

    int row = 0;

    // Fill holes
    fillHolesCheck = new QCheckBox("Заполнять дырки в масках");
    fillHolesCheck->setChecked(true);
    fillHolesCheck->setToolTip("Заполнять внутренние пустоты в детектированных клетках");
    connect(fillHolesCheck, &QCheckBox::stateChanged,
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(fillHolesCheck, row, 0, 1, 3);
    row++;

    // Morph kernel size
    morphKernelSizeSpin = new QSpinBox();
    morphKernelSizeSpin->setRange(0, 15);
    morphKernelSizeSpin->setSingleStep(2);
    morphKernelSizeSpin->setValue(3);
    morphKernelSizeSpin->setToolTip("Размер ядра для морфологических операций (0 = отключено)");
    connect(morphKernelSizeSpin, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("Размер ядра морф. операций:"), row, 0);
    layout->addWidget(morphKernelSizeSpin, row, 1);
    row++;

    // Min circularity
    minCircularitySpin = new QDoubleSpinBox();
    minCircularitySpin->setRange(0.0, 1.0);
    minCircularitySpin->setSingleStep(0.05);
    minCircularitySpin->setDecimals(2);
    minCircularitySpin->setValue(0.0);
    minCircularitySpin->setToolTip("Минимальная круглость клеток (0.0 = отключено)");
    connect(minCircularitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("Мин. круглость:"), row, 0);
    layout->addWidget(minCircularitySpin, row, 1);
    row++;

    // Max circularity
    maxCircularitySpin = new QDoubleSpinBox();
    maxCircularitySpin->setRange(0.0, 1.0);
    maxCircularitySpin->setSingleStep(0.05);
    maxCircularitySpin->setDecimals(2);
    maxCircularitySpin->setValue(1.0);
    maxCircularitySpin->setToolTip("Максимальная круглость клеток");
    connect(maxCircularitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(new QLabel("Макс. круглость:"), row, 0);
    layout->addWidget(maxCircularitySpin, row, 1);
    row++;
}

void NeuralNetParametersWidget::createGPUSettingsGroup() {
    gpuGroup = new QGroupBox("GPU настройки");
    QVBoxLayout* layout = new QVBoxLayout(gpuGroup);

    // Use GPU checkbox
    useGPUCheck = new QCheckBox("Использовать GPU (CUDA)");
    useGPUCheck->setChecked(false);
    useGPUCheck->setToolTip("Использовать GPU для ускорения инференса (требуется CUDA)");
    connect(useGPUCheck, &QCheckBox::stateChanged,
            this, &NeuralNetParametersWidget::onParameterChanged);
    layout->addWidget(useGPUCheck);

    // GPU device selection
    QHBoxLayout* gpuDeviceLayout = new QHBoxLayout();
    gpuDeviceLayout->addWidget(new QLabel("GPU устройство:"));
    gpuDeviceCombo = new QComboBox();
    gpuDeviceCombo->setEnabled(false); // Будет включено после обнаружения GPU
    gpuDeviceLayout->addWidget(gpuDeviceCombo, 1);
    layout->addLayout(gpuDeviceLayout);

    // Test GPU button
    testGPUButton = new QPushButton("🔍 Проверить доступность GPU");
    connect(testGPUButton, &QPushButton::clicked, this, &NeuralNetParametersWidget::onTestGPU);
    layout->addWidget(testGPUButton);

    // GPU status
    gpuStatusLabel = new QLabel("GPU статус: не проверен");
    gpuStatusLabel->setWordWrap(true);
    layout->addWidget(gpuStatusLabel);

    // Автоматическая проверка при создании
    onTestGPU();
}

void NeuralNetParametersWidget::onBrowseModel() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "Выбрать ONNX модель",
        QString(),
        "ONNX Models (*.onnx);;All Files (*.*)"
    );

    if (!fileName.isEmpty()) {
        modelPathEdit->setText(fileName);
    }
}

void NeuralNetParametersWidget::onLoadModel() {
    QString modelPath = modelPathEdit->text();

    if (modelPath.isEmpty()) {
        QMessageBox::warning(this, "Ошибка", "Пожалуйста, выберите ONNX модель");
        return;
    }

    LOG_INFO("Loading model: " + modelPath);

    bool useGPU = useGPUCheck->isChecked();
    bool success = detector->loadModel(modelPath.toStdString(), useGPU);

    if (success) {
        m_modelValid = true;
        modelStatusLabel->setText("✓ Модель загружена успешно");
        modelStatusLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        m_currentParams.modelPath = modelPath.toStdString();

        LOG_INFO("Model loaded successfully: " + modelPath);
        emit modelLoaded(true);
    } else {
        m_modelValid = false;
        modelStatusLabel->setText("✗ Ошибка загрузки модели");
        modelStatusLabel->setStyleSheet("color: #f44336; font-weight: bold;");

        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить модель.\nПроверьте лог для деталей.");
        emit modelLoaded(false);
    }
}

void NeuralNetParametersWidget::onParameterChanged() {
    m_currentParams.inputSize = inputSizeSpin->value();
    m_currentParams.confidenceThreshold = static_cast<float>(confidenceSpin->value());
    m_currentParams.minCellSize = minCellSizeSpin->value();
    m_currentParams.maxCellSize = maxCellSizeSpin->value();
    m_currentParams.nmsThreshold = static_cast<float>(nmsThresholdSpin->value());
    m_currentParams.useGPU = useGPUCheck->isChecked();
    m_currentParams.fillHoles = fillHolesCheck->isChecked();
    m_currentParams.morphKernelSize = morphKernelSizeSpin->value();
    m_currentParams.minCircularity = minCircularitySpin->value();
    m_currentParams.maxCircularity = maxCircularitySpin->value();

    emit parametersChanged();
}

void NeuralNetParametersWidget::onNumClassesChanged(int numClasses) {
    m_currentParams.numClasses = numClasses;
    updateClassMappingTable();
    emit parametersChanged();
}

void NeuralNetParametersWidget::updateClassMappingTable() {
    // Очистка предыдущих виджетов
    QLayoutItem* item;
    while ((item = classMappingLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
    classNameEdits.clear();

    int numClasses = numClassesSpin->value();

    // Создание строк для каждого класса
    for (int i = 1; i <= numClasses; i++) {
        QHBoxLayout* rowLayout = new QHBoxLayout();

        QLabel* classLabel = new QLabel(QString("Класс %1:").arg(i));
        classLabel->setMinimumWidth(70);

        QLineEdit* nameEdit = new QLineEdit();
        nameEdit->setPlaceholderText(QString("Название типа клетки %1...").arg(i));

        // Загрузка существующего названия
        if (m_currentParams.classNames.contains(i)) {
            nameEdit->setText(m_currentParams.classNames[i]);
        } else {
            nameEdit->setText(QString("Type %1").arg(QChar('A' + i - 1)));
        }

        connect(nameEdit, &QLineEdit::textChanged, [this, i, nameEdit]() {
            m_currentParams.classNames[i] = nameEdit->text();
            emit parametersChanged();
        });

        classNameEdits[i] = nameEdit;

        rowLayout->addWidget(classLabel);
        rowLayout->addWidget(nameEdit, 1);

        classMappingLayout->addLayout(rowLayout);
    }

    // Обновление параметров
    loadClassMappingsFromUI();
}

void NeuralNetParametersWidget::loadClassMappingsFromUI() {
    m_currentParams.classNames.clear();

    for (auto it = classNameEdits.begin(); it != classNameEdits.end(); ++it) {
        int classId = it.key();
        QString className = it.value()->text();
        m_currentParams.classNames[classId] = className;
    }
}

void NeuralNetParametersWidget::onAddClassMapping() {
    // Увеличиваем количество классов
    numClassesSpin->setValue(numClassesSpin->value() + 1);
}

void NeuralNetParametersWidget::onRemoveClassMapping() {
    // Уменьшаем количество классов
    if (numClassesSpin->value() > 1) {
        numClassesSpin->setValue(numClassesSpin->value() - 1);
    }
}

void NeuralNetParametersWidget::onTestGPU() {
    bool cudaAvailable = NeuralNetDetector::isCudaAvailable();

    if (cudaAvailable) {
        QVector<QString> gpus = NeuralNetDetector::getAvailableGPUs();
        gpuStatusLabel->setText(QString("✓ CUDA доступна\nНайдено устройств: %1").arg(gpus.size()));
        gpuStatusLabel->setStyleSheet("color: #4CAF50;");

        gpuDeviceCombo->clear();
        for (const QString& gpu : gpus) {
            gpuDeviceCombo->addItem(gpu);
        }
        gpuDeviceCombo->setEnabled(true);
        useGPUCheck->setEnabled(true);
    } else {
        gpuStatusLabel->setText("✗ CUDA недоступна\nБудет использоваться CPU");
        gpuStatusLabel->setStyleSheet("color: #f44336;");
        gpuDeviceCombo->setEnabled(false);
        useGPUCheck->setEnabled(false);
        useGPUCheck->setChecked(false);
    }
}

NeuralNetDetector::NeuralNetParams NeuralNetParametersWidget::getParameters() const {
    return m_currentParams;
}

void NeuralNetParametersWidget::setParameters(const NeuralNetDetector::NeuralNetParams& params) {
    m_currentParams = params;

    // Обновление UI
    modelPathEdit->setText(QString::fromStdString(params.modelPath));
    inputSizeSpin->setValue(params.inputSize);
    confidenceSpin->setValue(params.confidenceThreshold);
    minCellSizeSpin->setValue(params.minCellSize);
    maxCellSizeSpin->setValue(params.maxCellSize);
    nmsThresholdSpin->setValue(params.nmsThreshold);
    useGPUCheck->setChecked(params.useGPU);
    numClassesSpin->setValue(params.numClasses);
    fillHolesCheck->setChecked(params.fillHoles);
    morphKernelSizeSpin->setValue(params.morphKernelSize);
    minCircularitySpin->setValue(params.minCircularity);
    maxCircularitySpin->setValue(params.maxCircularity);

    // Обновление таблицы классов
    updateClassMappingTable();
}

bool NeuralNetParametersWidget::isValid() const {
    return m_modelValid && !m_currentParams.modelPath.empty();
}

QString NeuralNetParametersWidget::getModelPath() const {
    return QString::fromStdString(m_currentParams.modelPath);
}
