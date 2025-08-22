#include "parametertuningwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QInputDialog>
#include <QSettings>
#include <QPixmap>
#include <QScrollArea>
#include <QImage>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <limits>
#include <functional>
#include "settingsmanager.h"
#include "logger.h"

ParameterTuningWidget::ParameterTuningWidget(const QString& imagePath, QWidget *parent)
    : QWidget(parent), m_imagePath(imagePath) {
    
    m_originalImage = loadImageSafely(imagePath);
    if (m_originalImage.empty()) {
        QMessageBox::critical(this, "Ошибка", 
            QString("Не удалось загрузить изображение:\n%1\n\nВозможные причины:\n"
                   "- Файл не существует\n"
                   "- Неподдерживаемый формат\n"
                   "- Проблемы с правами доступа\n"
                   "- Кириллические символы в пути")
            .arg(imagePath));
        return;
    }
    
    cv::cvtColor(m_originalImage, m_grayImage, cv::COLOR_BGR2GRAY);
    cv::medianBlur(m_grayImage, m_blurredImage, 5);
    
    // Загружаем параметры из настроек
    m_currentParams = SettingsManager::instance().getHoughParams();
    
    // Инициализируем коэффициенты масштабирования и состояние
    m_scaleFactorX = 1.0;
    m_scaleFactorY = 1.0;
    m_parametersApplied = false;
    
    // Инициализируем таймер для задержанного применения параметров
    m_parameterUpdateTimer = new QTimer(this);
    m_parameterUpdateTimer->setSingleShot(true);
    m_parameterUpdateTimer->setInterval(200); // 0.2 секунды
    connect(m_parameterUpdateTimer, &QTimer::timeout, this, &ParameterTuningWidget::onParameterChangedDelayed);
    
    setupUI();
    loadPresets();
    // Показываем только оригинальное изображение без разметки
    showOriginalImage();
}

void ParameterTuningWidget::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // Заголовок
    auto* titleLabel = new QLabel("Настройка параметров HoughCircles");
    titleLabel->setAlignment(Qt::AlignCenter);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    mainLayout->addWidget(titleLabel);
    
    // Горизонтальный layout для изображения и параметров
    auto* contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(20);
    
    // Область для предпросмотра (слева)
    auto* previewContainer = new QWidget();
    auto* previewLayout = new QVBoxLayout(previewContainer);
    
    // Инструкция по использованию
    auto* instructionLabel = new QLabel("💡 Кликайте по клеткам, которые нужно обнаружить");
    instructionLabel->setStyleSheet("QLabel { color: #2196F3; font-weight: bold; padding: 5px; }");
    previewLayout->addWidget(instructionLabel);
    
    m_previewLabel = new InteractiveImageLabel();
    m_previewLabel->setMinimumSize(600, 450);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_previewLabel->setScaledContents(false);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("QLabel { border: 1px solid black; background-color: white; cursor: crosshair; }");
    connect(m_previewLabel, &InteractiveImageLabel::imageClicked, this, &ParameterTuningWidget::onImageClicked);
    previewLayout->addWidget(m_previewLabel);
    
    // Информация о выборе и кнопки управления
    auto* selectionControlsLayout = new QHBoxLayout();
    m_selectionInfoLabel = new QLabel("Выбрано клеток: 0");
    m_selectionInfoLabel->setStyleSheet("QLabel { font-weight: bold; }");
    selectionControlsLayout->addWidget(m_selectionInfoLabel);
    
    m_clearSelectionButton = new QPushButton("Очистить выбор");
    m_clearSelectionButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; border-radius: 10px; padding: 5px; }"
        "QPushButton:hover { background-color: #d32f2f; }"
    );
    connect(m_clearSelectionButton, &QPushButton::clicked, this, &ParameterTuningWidget::onClearSelection);
    selectionControlsLayout->addWidget(m_clearSelectionButton);
    
    m_autoFitButton = new QPushButton("🎯 Подобрать параметры");
    m_autoFitButton->setStyleSheet(
        "QPushButton { background-color: #FF9800; color: white; border-radius: 10px; padding: 5px; font-weight: bold; }"
        "QPushButton:hover { background-color: #F57C00; }"
    );
    m_autoFitButton->setEnabled(false);
    connect(m_autoFitButton, &QPushButton::clicked, this, &ParameterTuningWidget::onAutoFitParameters);
    selectionControlsLayout->addWidget(m_autoFitButton);
    
    previewLayout->addLayout(selectionControlsLayout);
    contentLayout->addWidget(previewContainer, 2); // Больше места для изображения
    
    // Панель параметров (справа)
    auto* paramsContainer = new QWidget();
    paramsContainer->setMaximumWidth(400);
    auto* paramsVLayout = new QVBoxLayout(paramsContainer);
    
    // Группа параметров
    auto* paramsGroup = new QGroupBox("Параметры HoughCircles");
    auto* paramsLayout = new QGridLayout(paramsGroup);
    
    // dp
    paramsLayout->addWidget(new QLabel("dp (разрешение аккумулятора):"), 0, 0);
    m_dpSpinBox = new QDoubleSpinBox();
    m_dpSpinBox->setRange(0.1, 10.0);
    m_dpSpinBox->setSingleStep(0.1);
    m_dpSpinBox->setValue(m_currentParams.dp);
    m_dpSpinBox->setToolTip("Обратное отношение разрешения аккумулятора к разрешению изображения");
    paramsLayout->addWidget(m_dpSpinBox, 0, 1);
    
    // minDist
    paramsLayout->addWidget(new QLabel("minDist (мин. расстояние):"), 1, 0);
    m_minDistSpinBox = new QDoubleSpinBox();
    m_minDistSpinBox->setRange(1.0, 500.0);
    m_minDistSpinBox->setSingleStep(1.0);
    m_minDistSpinBox->setValue(m_currentParams.minDist);
    m_minDistSpinBox->setToolTip("Минимальное расстояние между центрами обнаруженных кругов");
    paramsLayout->addWidget(m_minDistSpinBox, 1, 1);
    
    // param1
    paramsLayout->addWidget(new QLabel("param1 (порог Canny):"), 2, 0);
    m_param1SpinBox = new QDoubleSpinBox();
    m_param1SpinBox->setRange(1.0, 300.0);
    m_param1SpinBox->setSingleStep(1.0);
    m_param1SpinBox->setValue(m_currentParams.param1);
    m_param1SpinBox->setToolTip("Верхний порог для детектора границ Canny");
    paramsLayout->addWidget(m_param1SpinBox, 2, 1);
    
    // param2
    paramsLayout->addWidget(new QLabel("param2 (порог центра):"), 3, 0);
    m_param2SpinBox = new QDoubleSpinBox();
    m_param2SpinBox->setRange(1.0, 300.0);
    m_param2SpinBox->setSingleStep(1.0);
    m_param2SpinBox->setValue(m_currentParams.param2);
    m_param2SpinBox->setToolTip("Порог для центра круга в процессе обнаружения");
    paramsLayout->addWidget(m_param2SpinBox, 3, 1);
    
    // minRadius
    paramsLayout->addWidget(new QLabel("minRadius (мин. радиус):"), 4, 0);
    m_minRadiusSpinBox = new QSpinBox();
    m_minRadiusSpinBox->setRange(1, 500);
    m_minRadiusSpinBox->setValue(m_currentParams.minRadius);
    m_minRadiusSpinBox->setToolTip("Минимальный радиус круга для обнаружения");
    paramsLayout->addWidget(m_minRadiusSpinBox, 4, 1);
    
    // maxRadius
    paramsLayout->addWidget(new QLabel("maxRadius (макс. радиус):"), 5, 0);
    m_maxRadiusSpinBox = new QSpinBox();
    m_maxRadiusSpinBox->setRange(1, 1000);
    m_maxRadiusSpinBox->setValue(m_currentParams.maxRadius);
    m_maxRadiusSpinBox->setToolTip("Максимальный радиус круга для обнаружения");
    paramsLayout->addWidget(m_maxRadiusSpinBox, 5, 1);
    
    paramsVLayout->addWidget(paramsGroup);
    
    // Пресеты
    auto* presetGroup = new QGroupBox("Наборы параметров");
    auto* presetLayout = new QVBoxLayout(presetGroup);
    
    m_presetCombo = new QComboBox();
    m_presetCombo->addItem("По умолчанию");
    presetLayout->addWidget(m_presetCombo);
    
    // Горизонтальный макет для кнопок пресетов
    auto* presetButtonsLayout = new QHBoxLayout();
    
    auto* savePresetBtn = new QPushButton("💾 Сохранить");
    savePresetBtn->setStyleSheet("QPushButton { background-color: #2196F3; color: white; border-radius: 5px; padding: 5px; }");
    connect(savePresetBtn, &QPushButton::clicked, this, &ParameterTuningWidget::onSavePreset);
    presetButtonsLayout->addWidget(savePresetBtn);
    
    auto* deletePresetBtn = new QPushButton("🗑️ Удалить");
    deletePresetBtn->setStyleSheet("QPushButton { background-color: #f44336; color: white; border-radius: 5px; padding: 5px; }");
    connect(deletePresetBtn, &QPushButton::clicked, this, &ParameterTuningWidget::onDeletePreset);
    presetButtonsLayout->addWidget(deletePresetBtn);
    
    presetLayout->addLayout(presetButtonsLayout);
    
    paramsVLayout->addWidget(presetGroup);
    
    // Кнопка применения параметров
    m_applyButton = new QPushButton("Применить параметры");
    m_applyButton->setMinimumHeight(40);
    m_applyButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; font-weight: bold; border-radius: 10px; }"
        "QPushButton:hover { background-color: #1976D2; }"
    );
    connect(m_applyButton, &QPushButton::clicked, this, &ParameterTuningWidget::onApplyParameters);
    paramsVLayout->addWidget(m_applyButton);
    
    paramsVLayout->addStretch();
    
    contentLayout->addWidget(paramsContainer, 1);
    mainLayout->addLayout(contentLayout);
    
    // Нижний тулбар с кнопками
    auto* bottomLayout = new QHBoxLayout();
    
    // Кнопка "Назад" слева
    auto* backButton = new QPushButton("← Назад");
    backButton->setMinimumHeight(40);
    backButton->setMinimumWidth(100);
    backButton->setStyleSheet(
        "QPushButton { background-color: #757575; color: white; font-weight: bold; border-radius: 10px; padding: 10px; }"
        "QPushButton:hover { background-color: #616161; }"
    );
    connect(backButton, &QPushButton::clicked, this, &QWidget::close);
    bottomLayout->addWidget(backButton);
    
    // Кнопка "Сбросить всё" в центре слева
    auto* resetAllButton = new QPushButton("🔄 Сбросить всё");
    resetAllButton->setMinimumHeight(40);
    resetAllButton->setMinimumWidth(120);
    resetAllButton->setStyleSheet(
        "QPushButton { background-color: #9C27B0; color: white; font-weight: bold; border-radius: 10px; padding: 10px; }"
        "QPushButton:hover { background-color: #7B1FA2; }"
    );
    connect(resetAllButton, &QPushButton::clicked, this, &ParameterTuningWidget::onResetAll);
    bottomLayout->addWidget(resetAllButton);
    
    bottomLayout->addStretch(); // Растягивающий элемент в центре
    
    // Кнопка "Продолжить" справа
    m_confirmButton = new QPushButton("Продолжить");
    m_confirmButton->setMinimumHeight(40);
    m_confirmButton->setMinimumWidth(120);
    m_confirmButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-weight: bold; border-radius: 10px; padding: 10px; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #cccccc; color: #666666; }"
    );
    m_confirmButton->setEnabled(false); // Отключена до применения параметров
    connect(m_confirmButton, &QPushButton::clicked, this, &ParameterTuningWidget::onConfirmClicked);
    bottomLayout->addWidget(m_confirmButton);
    
    mainLayout->addLayout(bottomLayout);
    
    // Подключаем сигналы изменения параметров
    connect(m_dpSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParameterTuningWidget::onParameterChanged);
    connect(m_minDistSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParameterTuningWidget::onParameterChanged);
    connect(m_param1SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParameterTuningWidget::onParameterChanged);
    connect(m_param2SpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &ParameterTuningWidget::onParameterChanged);
    connect(m_minRadiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ParameterTuningWidget::onParameterChanged);
    connect(m_maxRadiusSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &ParameterTuningWidget::onParameterChanged);
    connect(m_presetCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ParameterTuningWidget::onLoadPreset);
}

void ParameterTuningWidget::onParameterChanged() {
    // Обновляем параметры
    m_currentParams.dp = m_dpSpinBox->value();
    m_currentParams.minDist = m_minDistSpinBox->value();
    m_currentParams.param1 = m_param1SpinBox->value();
    m_currentParams.param2 = m_param2SpinBox->value();
    m_currentParams.minRadius = m_minRadiusSpinBox->value();
    m_currentParams.maxRadius = m_maxRadiusSpinBox->value();
    
    // Перезапускаем таймер для задержанного применения
    m_parameterUpdateTimer->start();
}

void ParameterTuningWidget::onParameterChangedDelayed() {
    // Автоматически применяем параметры после задержки
    if (!m_parametersApplied) {
        m_parametersApplied = true;
        m_confirmButton->setEnabled(true);
    }
    
    // Обновляем превью
    updatePreview();
}

void ParameterTuningWidget::onApplyParameters() {
    m_parametersApplied = true;
    m_confirmButton->setEnabled(true);
    updatePreview();
}

void ParameterTuningWidget::showOriginalImage() {
    if (m_originalImage.empty()) {
        return;
    }
    
    cv::Mat preview = m_originalImage.clone();
    
    // Рисуем только выбранные пользователем клетки (красные точки)
    drawSelectedCells(preview);
    
    // Конвертируем в QPixmap и отображаем
    cv::Mat rgb;
    cv::cvtColor(preview, rgb, cv::COLOR_BGR2RGB);
    QImage img = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    QPixmap pixmap = QPixmap::fromImage(img);
    
    // Фиксированные размеры для стабильного масштабирования
    const int targetWidth = 800;
    const int targetHeight = 600;
    
    QSize originalSize = pixmap.size();
    if (originalSize.width() > targetWidth || originalSize.height() > targetHeight) {
        pixmap = pixmap.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Сохраняем коэффициенты масштабирования для преобразования координат
    m_scaledImageSize = pixmap.size();
    m_scaleFactorX = double(originalSize.width()) / double(pixmap.width());
    m_scaleFactorY = double(originalSize.height()) / double(pixmap.height());
    
    m_previewLabel->setImageSize(m_scaledImageSize);
    m_previewLabel->setPixmap(pixmap);
}

void ParameterTuningWidget::updatePreview() {
    if (m_originalImage.empty() || m_blurredImage.empty()) {
        return;
    }
    
    cv::Mat preview = m_originalImage.clone();
    std::vector<cv::Vec3f> circles;
    
    try {
        cv::HoughCircles(
            m_blurredImage, circles,
            cv::HOUGH_GRADIENT,
            m_currentParams.dp,
            m_currentParams.minDist,
            m_currentParams.param1,
            m_currentParams.param2,
            m_currentParams.minRadius,
            m_currentParams.maxRadius
        );
    } catch (const cv::Exception& e) {
        QMessageBox::warning(this, "Предупреждение", 
            QString("Ошибка при обнаружении кругов: %1").arg(e.what()));
        return;
    }
    
    // Фильтруем перекрывающиеся круги с учетом minDist
    std::vector<cv::Vec3f> filteredCircles = filterOverlappingCircles(circles, m_currentParams.minDist);
    
    // Рисуем найденные круги
    for (const auto& c : filteredCircles) {
        int x = cvRound(c[0]);
        int y = cvRound(c[1]);
        int r = cvRound(c[2]);
        
        // Проверяем границы
        if (x - r >= 0 && y - r >= 0 && 
            x + r < preview.cols && y + r < preview.rows) {
            // Рисуем красный прямоугольник вокруг круга
            cv::Rect rect(x - r, y - r, 2 * r, 2 * r);
            cv::rectangle(preview, rect, cv::Scalar(0, 0, 255), 2);
        }
    }
    
    // Добавляем информацию о количестве найденных кругов
    std::string info = "Found: " + std::to_string(circles.size()) + " -> Filtered: " + std::to_string(filteredCircles.size());
    cv::putText(preview, info, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 
                0.8, cv::Scalar(0, 255, 0), 2);
    
    // Конвертируем в QPixmap и отображаем
    cv::Mat rgb;
    cv::cvtColor(preview, rgb, cv::COLOR_BGR2RGB);
    QImage img = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    QPixmap pixmap = QPixmap::fromImage(img);
    
    // Используем те же фиксированные размеры для стабильности
    const int targetWidth = 800;
    const int targetHeight = 600;
    
    QSize originalSize = pixmap.size();
    if (originalSize.width() > targetWidth || originalSize.height() > targetHeight) {
        pixmap = pixmap.scaled(targetWidth, targetHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    // Обновляем коэффициенты масштабирования (если они изменились)
    m_scaledImageSize = pixmap.size();
    m_scaleFactorX = double(originalSize.width()) / double(pixmap.width());
    m_scaleFactorY = double(originalSize.height()) / double(pixmap.height());
    
    m_previewLabel->setImageSize(m_scaledImageSize);
    m_previewLabel->setPixmap(pixmap);
}

void ParameterTuningWidget::onConfirmClicked() {
    LOG_INFO("ParameterTuningWidget::onConfirmClicked() called");
    LOG_INFO(QString("Parameters: dp=%1, minDist=%2, param1=%3, param2=%4, minRadius=%5, maxRadius=%6")
        .arg(m_currentParams.dp)
        .arg(m_currentParams.minDist)
        .arg(m_currentParams.param1)
        .arg(m_currentParams.param2)
        .arg(m_currentParams.minRadius)
        .arg(m_currentParams.maxRadius));
    
    emit parametersConfirmed(m_currentParams);
    
    LOG_INFO("parametersConfirmed signal emitted");
}

void ParameterTuningWidget::onSavePreset() {
    bool ok;
    QString name = QInputDialog::getText(this, "Сохранить набор параметров",
                                       "Введите название:", QLineEdit::Normal,
                                       "", &ok);
    if (ok && !name.isEmpty()) {
        // Проверяем, существует ли уже пресет с таким именем
        if (m_presets.contains(name)) {
            int ret = QMessageBox::question(this, "Подтверждение", 
                QString("Набор параметров '%1' уже существует. Заменить?").arg(name),
                QMessageBox::Yes | QMessageBox::No);
            if (ret != QMessageBox::Yes) {
                return;
            }
            // Удаляем старый элемент из комбобокса
            int index = m_presetCombo->findText(name);
            if (index != -1) {
                m_presetCombo->removeItem(index);
            }
        }
        
        // Получаем текущий коэффициент из SettingsManager
        double currentCoeff = SettingsManager::instance().getNmPerPixel();
        m_presets[name] = PresetData(m_currentParams, currentCoeff);
        m_presetCombo->addItem(name);
        m_presetCombo->setCurrentText(name);
        savePresets();
        
        LOG_INFO(QString("Сохранен пресет '%1' с коэффициентом %2 нм/px").arg(name).arg(currentCoeff));
    }
}

void ParameterTuningWidget::onLoadPreset() {
    QString currentText = m_presetCombo->currentText();
    double presetCoeff = 0.0;
    
    if (currentText == "По умолчанию") {
        m_currentParams = HoughParams();
        presetCoeff = 0.0;
    } else if (m_presets.contains(currentText)) {
        const PresetData& presetData = m_presets[currentText];
        m_currentParams = presetData.params;
        presetCoeff = presetData.coefficient;
        
        // Если в пресете есть коэффициент, применяем его
        if (presetCoeff > 0.0) {
            SettingsManager::instance().setNmPerPixel(presetCoeff);
            LOG_INFO(QString("Загружен пресет '%1' с коэффициентом %2 нм/px").arg(currentText).arg(presetCoeff));
        }
    }
    
    // Обновляем значения в спинбоксах
    m_dpSpinBox->setValue(m_currentParams.dp);
    m_minDistSpinBox->setValue(m_currentParams.minDist);
    m_param1SpinBox->setValue(m_currentParams.param1);
    m_param2SpinBox->setValue(m_currentParams.param2);
    m_minRadiusSpinBox->setValue(m_currentParams.minRadius);
    m_maxRadiusSpinBox->setValue(m_currentParams.maxRadius);
    
    // АВТОМАТИЧЕСКИ применяем параметры при загрузке пресета
    m_parametersApplied = true;
    m_confirmButton->setEnabled(true);
    updatePreview(); // Применяем параметры сразу
    
    LOG_INFO(QString("Пресет '%1' применен автоматически").arg(currentText));
}

void ParameterTuningWidget::onDeletePreset() {
    QString currentText = m_presetCombo->currentText();
    
    // Нельзя удалить "По умолчанию"
    if (currentText == "По умолчанию") {
        QMessageBox::information(this, "Информация", "Нельзя удалить встроенный набор параметров 'По умолчанию'");
        return;
    }
    
    if (!m_presets.contains(currentText)) {
        return;
    }
    
    // Подтверждение удаления
    int ret = QMessageBox::question(this, "Подтверждение удаления",
        QString("Вы уверены что хотите удалить набор параметров '%1'?").arg(currentText),
        QMessageBox::Yes | QMessageBox::No);
        
    if (ret == QMessageBox::Yes) {
        // Удаляем из карты и комбобокса
        m_presets.remove(currentText);
        int index = m_presetCombo->findText(currentText);
        if (index != -1) {
            m_presetCombo->removeItem(index);
        }
        
        // Переключаемся на "По умолчанию"
        m_presetCombo->setCurrentText("По умолчанию");
        onLoadPreset(); // Загружаем параметры по умолчанию
        
        savePresets();
        LOG_INFO(QString("Пресет '%1' удален").arg(currentText));
    }
}

void ParameterTuningWidget::loadPresets() {
    QSettings settings("CellAnalyzer", "HoughParams");
    int size = settings.beginReadArray("presets");
    for (int i = 0; i < size; ++i) {
        settings.setArrayIndex(i);
        QString name = settings.value("name").toString();
        HoughParams params;
        params.dp = settings.value("dp", 1.0).toDouble();
        params.minDist = settings.value("minDist", 30.0).toDouble();
        params.param1 = settings.value("param1", 80.0).toDouble();
        params.param2 = settings.value("param2", 40.0).toDouble();
        params.minRadius = settings.value("minRadius", 30).toInt();
        params.maxRadius = settings.value("maxRadius", 130).toInt();
        
        double coefficient = settings.value("coefficient", 0.0).toDouble();
        m_presets[name] = PresetData(params, coefficient);
        m_presetCombo->addItem(name);
    }
    settings.endArray();
    
    // Загружаем последний выбранный пресет
    QString lastPreset = settings.value("lastSelectedPreset", "По умолчанию").toString();
    int index = m_presetCombo->findText(lastPreset);
    if (index != -1) {
        m_presetCombo->setCurrentIndex(index);
        onLoadPreset(); // Автоматически загружаем последний пресет
    }
}

void ParameterTuningWidget::savePresets() {
    QSettings settings("CellAnalyzer", "HoughParams");
    settings.beginWriteArray("presets");
    int i = 0;
    for (auto it = m_presets.begin(); it != m_presets.end(); ++it, ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", it.key());
        settings.setValue("dp", it.value().params.dp);
        settings.setValue("minDist", it.value().params.minDist);
        settings.setValue("param1", it.value().params.param1);
        settings.setValue("param2", it.value().params.param2);
        settings.setValue("minRadius", it.value().params.minRadius);
        settings.setValue("maxRadius", it.value().params.maxRadius);
        settings.setValue("coefficient", it.value().coefficient);
    }
    settings.endArray();
    
    // Сохраняем последний выбранный пресет
    settings.setValue("lastSelectedPreset", m_presetCombo->currentText());
}

void ParameterTuningWidget::onImageClicked(QPoint position) {
    // position уже в координатах отображаемого изображения, преобразуем в координаты оригинала
    QPoint imagePos;
    imagePos.setX(int(position.x() * m_scaleFactorX));
    imagePos.setY(int(position.y() * m_scaleFactorY));
    
    // Проверяем границы оригинального изображения
    if (imagePos.x() < 0 || imagePos.y() < 0 || 
        imagePos.x() >= m_originalImage.cols || imagePos.y() >= m_originalImage.rows) {
        return; // Клик за пределами изображения
    }
    
    // Проверяем, не выбрана ли уже эта область (с допуском 20 пикселей)
    auto it = std::find_if(m_selectedCells.begin(), m_selectedCells.end(),
        [&imagePos](const cv::Point& p) {
            return std::abs(p.x - imagePos.x()) < 20 && std::abs(p.y - imagePos.y()) < 20;
        });
    
    if (it != m_selectedCells.end()) {
        // Удаляем точку, если она уже выбрана
        m_selectedCells.erase(it);
    } else {
        // Добавляем новую точку
        m_selectedCells.push_back(cv::Point(imagePos.x(), imagePos.y()));
    }
    
    // Обновляем информацию и кнопки
    m_selectionInfoLabel->setText(QString("Выбрано клеток: %1").arg(m_selectedCells.size()));
    m_autoFitButton->setEnabled(!m_selectedCells.empty());
    
    // Обновляем изображение
    if (m_parametersApplied) {
        updatePreview();
    } else {
        showOriginalImage();
    }
}

void ParameterTuningWidget::onClearSelection() {
    m_selectedCells.clear();
    m_selectionInfoLabel->setText("Выбрано клеток: 0");
    m_autoFitButton->setEnabled(false);
    
    // Обновляем изображение
    if (m_parametersApplied) {
        updatePreview();
    } else {
        showOriginalImage();
    }
}

void ParameterTuningWidget::onResetAll() {
    // Очищаем выбранные клетки
    m_selectedCells.clear();
    m_selectionInfoLabel->setText("Выбрано клеток: 0");
    m_autoFitButton->setEnabled(false);
    
    // Сбрасываем состояние применения параметров
    m_parametersApplied = false;
    
    // Сбрасываем параметры к значениям по умолчанию
    HoughParams defaultParams;
    m_currentParams = defaultParams;
    
    // Обновляем UI элементы с параметрами
    m_dpSpinBox->setValue(m_currentParams.dp);
    m_minDistSpinBox->setValue(m_currentParams.minDist);
    m_param1SpinBox->setValue(m_currentParams.param1);
    m_param2SpinBox->setValue(m_currentParams.param2);
    m_minRadiusSpinBox->setValue(m_currentParams.minRadius);
    m_maxRadiusSpinBox->setValue(m_currentParams.maxRadius);
    
    // Отключаем кнопку "Продолжить"
    m_confirmButton->setEnabled(false);
    
    // Показываем чистое изображение
    showOriginalImage();
}

void ParameterTuningWidget::onAutoFitParameters() {
    if (m_selectedCells.empty()) {
        QMessageBox::information(this, "Информация", "Сначала выберите клетки на изображении.");
        return;
    }
    
    QMessageBox::StandardButton reply = QMessageBox::question(this, "Подтверждение",
        QString("Подобрать параметры для %1 выбранных клеток?").arg(m_selectedCells.size()),
        QMessageBox::Yes | QMessageBox::No);
    
    if (reply == QMessageBox::Yes) {
        optimizeParametersForSelectedCells();
    }
}

void ParameterTuningWidget::drawSelectedCells(cv::Mat& image) {
    for (const auto& cell : m_selectedCells) {
        // Рисуем красную точку (заполненный круг) в месте клика
        cv::circle(image, cell, 8, cv::Scalar(0, 0, 255), -1); // -1 = заполненный круг
        // Рисуем белую окантовку для лучшей видимости
        cv::circle(image, cell, 8, cv::Scalar(255, 255, 255), 2);
    }
}

void ParameterTuningWidget::optimizeParametersForSelectedCells() {
    // Показываем прогресс
    auto startTime = std::chrono::steady_clock::now();
    
    HoughParams bestParams = findBestParametersForCells(m_selectedCells);
    
    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
    
    // Применяем найденные параметры
    m_currentParams = bestParams;
    
    // Обновляем UI
    m_dpSpinBox->setValue(m_currentParams.dp);
    m_minDistSpinBox->setValue(m_currentParams.minDist);
    m_param1SpinBox->setValue(m_currentParams.param1);
    m_param2SpinBox->setValue(m_currentParams.param2);
    m_minRadiusSpinBox->setValue(m_currentParams.minRadius);
    m_maxRadiusSpinBox->setValue(m_currentParams.maxRadius);
    
    // Автоматически применяем параметры
    m_parametersApplied = true;
    m_confirmButton->setEnabled(true);
    updatePreview();
    
    // Показываем результат пользователю
    if (elapsed.count() >= 60) {
        QMessageBox::warning(this, "Таймаут", 
            QString("Автоподбор завершен по таймауту (1 минута).\n"
                   "Параметры частично оптимизированы.\n"
                   "Если результат не устраивает, попробуйте настроить параметры вручную."));
    } else {
        QMessageBox::information(this, "Готово", 
            QString("Параметры автоматически подобраны за %1 сек.\n"
                   "Проверьте результат и при необходимости подкорректируйте вручную.")
                   .arg(elapsed.count()));
    }
}

ParameterTuningWidget::HoughParams ParameterTuningWidget::findBestParametersForCells(const std::vector<cv::Point>& selectedCells) {
    if (selectedCells.empty()) {
        return m_currentParams;
    }
    
    LOG_INFO(QString("СТРОГИЙ автоподбор параметров для %1 клеток").arg(selectedCells.size()));
    
    const auto startTime = std::chrono::steady_clock::now();
    const int timeoutSeconds = 60;
    
    // Стратегия: будем перебирать комбинации параметров до тех пор, 
    // пока не найдем такие, которые покрывают ВСЕ точки
    
    HoughParams bestParams = m_currentParams;
    double bestScore = 0.0;
    bool foundPerfectSolution = false;
    
    // Анализируем выбранные точки для определения диапазонов
    std::vector<double> distances;
    for (size_t i = 0; i < selectedCells.size(); ++i) {
        for (size_t j = i + 1; j < selectedCells.size(); ++j) {
            cv::Point diff = selectedCells[i] - selectedCells[j];
            double distance = std::sqrt(diff.x * diff.x + diff.y * diff.y);
            distances.push_back(distance);
        }
    }
    
    double minDistBetweenCells = distances.empty() ? 30.0 : *std::min_element(distances.begin(), distances.end());
    LOG_INFO(QString("Минимальное расстояние между точками: %1").arg(minDistBetweenCells));
    
    // Пробуем разные стратегии поиска до получения 100% покрытия
    std::vector<std::pair<QString, std::function<std::vector<HoughParams>()>>> strategies;
    
    // Стратегия 1: Разные радиусы с агрессивными параметрами обнаружения
    strategies.push_back({"Агрессивное обнаружение", [&]() -> std::vector<HoughParams> {
        std::vector<HoughParams> candidates;
        for (int minR = 15; minR <= 35; minR += 5) {
            for (int maxR = minR + 30; maxR <= minR + 80; maxR += 10) {
                for (double param2 = 15.0; param2 <= 35.0; param2 += 5.0) {
                    HoughParams candidate = m_currentParams;
                    candidate.minRadius = minR;
                    candidate.maxRadius = maxR;
                    candidate.param1 = 50.0; // Низкий порог для более агрессивного поиска
                    candidate.param2 = param2; // Низкий порог накопления
                    candidate.minDist = std::max(5.0, minDistBetweenCells * 0.6);
                    candidates.push_back(candidate);
                }
            }
        }
        return candidates;
    }});
    
    // Стратегия 2: Различные пороги с фиксированными радиусами
    strategies.push_back({"Варьирование порогов", [&]() -> std::vector<HoughParams> {
        std::vector<HoughParams> candidates;
        int avgRadius = std::max(25, int(minDistBetweenCells / 2));
        for (double param1 = 30.0; param1 <= 120.0; param1 += 15.0) {
            for (double param2 = 10.0; param2 <= 50.0; param2 += 5.0) {
                HoughParams candidate = m_currentParams;
                candidate.minRadius = avgRadius - 10;
                candidate.maxRadius = avgRadius + 20;
                candidate.param1 = param1;
                candidate.param2 = param2;
                candidate.minDist = std::max(3.0, minDistBetweenCells * 0.4);
                candidates.push_back(candidate);
            }
        }
        return candidates;
    }});
    
    // Стратегия 3: Очень мелкие круги с низкими порогами
    strategies.push_back({"Мелкие круги", [&]() -> std::vector<HoughParams> {
        std::vector<HoughParams> candidates;
        for (int minR = 10; minR <= 25; minR += 3) {
            for (int maxR = minR + 20; maxR <= minR + 50; maxR += 10) {
                for (double param2 = 8.0; param2 <= 25.0; param2 += 3.0) {
                    HoughParams candidate = m_currentParams;
                    candidate.minRadius = minR;
                    candidate.maxRadius = maxR;
                    candidate.param1 = 40.0;
                    candidate.param2 = param2;
                    candidate.minDist = std::max(2.0, minDistBetweenCells * 0.3);
                    candidates.push_back(candidate);
                }
            }
        }
        return candidates;
    }});
    
    // Выполняем стратегии одну за другой
    for (const auto& strategy : strategies) {
        if (foundPerfectSolution) break;
        
        auto currentTime = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(currentTime - startTime);
        if (elapsed.count() >= timeoutSeconds) {
            LOG_INFO("Таймаут автоподбора параметров");
            break;
        }
        
        LOG_INFO(QString("Применяем стратегию: %1").arg(strategy.first));
        auto candidates = strategy.second();
        
        for (const auto& candidate : candidates) {
            double score = evaluateParametersForCells(candidate, selectedCells);
            
            // Проверяем, покрывают ли параметры ВСЕ точки
            if (score >= 1.0) { // 100% покрытие или больше
                LOG_INFO(QString("НАЙДЕНО ИДЕАЛЬНОЕ РЕШЕНИЕ! Score: %1").arg(score));
                bestParams = candidate;
                bestScore = score;
                foundPerfectSolution = true;
                break;
            }
            
            if (score > bestScore) {
                bestParams = candidate;
                bestScore = score;
                LOG_INFO(QString("Улучшение: score=%1 (minR=%2, maxR=%3, param1=%4, param2=%5)")
                    .arg(score).arg(candidate.minRadius).arg(candidate.maxRadius)
                    .arg(candidate.param1).arg(candidate.param2));
            }
        }
    }
    
    auto endTime = std::chrono::steady_clock::now();
    auto totalTime = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (foundPerfectSolution) {
        LOG_INFO(QString("УСПЕХ! Найдены параметры покрывающие все точки за %1мс").arg(totalTime.count()));
    } else {
        LOG_INFO(QString("Лучший результат: %1 за %2мс (может покрывать не все точки)").arg(bestScore).arg(totalTime.count()));
    }
    
    return bestParams;
}

double ParameterTuningWidget::evaluateParametersForCells(const HoughParams& params, const std::vector<cv::Point>& selectedCells) {
    if (selectedCells.empty()) return 0.0;
    
    std::vector<cv::Vec3f> circles;
    
    try {
        cv::HoughCircles(
            m_blurredImage, circles,
            cv::HOUGH_GRADIENT,
            params.dp,
            params.minDist,
            params.param1,
            params.param2,
            params.minRadius,
            params.maxRadius
        );
    } catch (const cv::Exception&) {
        return 0.0; // Некорректные параметры
    }
    
    if (circles.empty()) return 0.0;
    
    // Применяем фильтр для устранения наложений с учетом minDist
    std::vector<cv::Vec3f> filteredCircles = filterOverlappingCircles(circles, params.minDist);
    
    if (filteredCircles.empty()) return 0.0;
    
    // Каждая пользовательская точка должна попадать не более чем в один круг
    std::vector<bool> cellMatched(selectedCells.size(), false);
    std::vector<bool> circleUsed(filteredCircles.size(), false);
    int correctMatches = 0;
    
    // Максимальное расстояние для считания точки "внутри" круга (радиус круга)
    for (size_t i = 0; i < selectedCells.size(); ++i) {
        if (cellMatched[i]) continue;
        
        const auto& selectedCell = selectedCells[i];
        cv::Point2f cellPoint(selectedCell.x, selectedCell.y);
        
        double bestDistance = std::numeric_limits<double>::max();
        int bestCircleIdx = -1;
        
        // Ищем ближайший неиспользованный круг
        for (size_t j = 0; j < filteredCircles.size(); ++j) {
            if (circleUsed[j]) continue;
            
            const auto& circle = filteredCircles[j];
            cv::Point2f circleCenter(circle[0], circle[1]);
            double circleRadius = circle[2];
            double distance = cv::norm(cellPoint - circleCenter);
            
            // Точка должна быть внутри круга (с небольшим запасом)
            if (distance <= circleRadius * 1.2 && distance < bestDistance) {
                bestDistance = distance;
                bestCircleIdx = j;
            }
        }
        
        if (bestCircleIdx != -1) {
            cellMatched[i] = true;
            circleUsed[bestCircleIdx] = true;
            correctMatches++;
        }
    }
    
    // СТРОГОЕ ТРЕБОВАНИЕ: ВСЕ пользовательские точки ДОЛЖНЫ быть покрыты
    double coverage = double(correctMatches) / double(selectedCells.size());
    
    // Если не все точки покрыты - возвращаем очень низкую оценку
    if (correctMatches < selectedCells.size()) {
        // Частичное покрытие дает только базовую оценку
        return coverage * 0.1; // Максимум 10% от полной оценки
    }
    
    // Считаем ложные срабатывания (лишние круги)
    int unusedCircles = 0;
    for (bool used : circleUsed) {
        if (!used) unusedCircles++;
    }
    
    // Если все точки покрыты - считаем полную оценку
    double baseScore = 1.0; // 100% за полное покрытие
    
    // Штраф за ложные срабатывания (чем меньше лишних кругов, тем лучше)
    double efficiency = (unusedCircles == 0) ? 1.0 : 
                       std::max(0.3, 1.0 - (double(unusedCircles) / double(filteredCircles.size())));
    
    // Бонус за точное соответствие (количество кругов = количество точек)
    double precisionBonus = (filteredCircles.size() == selectedCells.size()) ? 0.2 : 0.0;
    
    double finalScore = baseScore * efficiency + precisionBonus;
    
    LOG_INFO(QString("Оценка параметров: покрыто %1/%2 точек, лишних кругов %3, итоговый счет %4")
        .arg(correctMatches).arg(selectedCells.size()).arg(unusedCircles).arg(finalScore));
    
    return finalScore;
}

cv::Mat ParameterTuningWidget::loadImageSafely(const QString& imagePath) {
    // Попробуем загрузить стандартным способом
    cv::Mat image = cv::imread(imagePath.toStdString());
    
    if (!image.empty()) {
        return image;
    }
    
    // Если не получилось, попробуем через QImage (лучше работает с Unicode)
    QImage qImage;
    if (!qImage.load(imagePath)) {
        LOG_ERROR("Не удалось загрузить изображение: " + imagePath);
        return cv::Mat();
    }
    
    // Конвертируем QImage в cv::Mat
    QImage rgbImage = qImage.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3, (void*)rgbImage.constBits(), rgbImage.bytesPerLine());
    cv::Mat result;
    cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);
    
    LOG_INFO("Изображение загружено через QImage: " + imagePath);
    return result.clone();
}

std::vector<cv::Vec3f> ParameterTuningWidget::filterOverlappingCircles(const std::vector<cv::Vec3f>& circles, double minDist) {
    if (circles.empty()) {
        return circles;
    }
    
    std::vector<cv::Vec3f> filtered;
    std::vector<bool> used(circles.size(), false);
    
    // Сортируем круги по убыванию силы отклика (acc - это третий параметр, но у нас его нет)
    // Используем радиус как критерий - большие круги имеют приоритет
    std::vector<std::pair<double, size_t>> sortedByRadius;
    for (size_t i = 0; i < circles.size(); ++i) {
        sortedByRadius.push_back({circles[i][2], i}); // радиус, индекс
    }
    std::sort(sortedByRadius.begin(), sortedByRadius.end(), std::greater<std::pair<double, size_t>>());
    
    for (const auto& pair : sortedByRadius) {
        size_t idx = pair.second;
        if (used[idx]) continue;
        
        const cv::Vec3f& currentCircle = circles[idx];
        cv::Point2f currentCenter(currentCircle[0], currentCircle[1]);
        
        // Проверяем, не слишком ли близко к уже выбранным кругам
        bool tooClose = false;
        for (const auto& acceptedCircle : filtered) {
            cv::Point2f acceptedCenter(acceptedCircle[0], acceptedCircle[1]);
            double distance = cv::norm(currentCenter - acceptedCenter);
            if (distance < minDist) {
                tooClose = true;
                break;
            }
        }
        
        if (!tooClose) {
            filtered.push_back(currentCircle);
            used[idx] = true;
        }
    }
    
    return filtered;
}

