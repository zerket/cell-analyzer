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
#include <QApplication>
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
    auto* instructionLabel = new QLabel("💡 ЛКМ - клетки для обнаружения (●)  |  ПКМ - объекты для исключения (✕)");
    instructionLabel->setStyleSheet("QLabel { color: #2196F3; font-weight: bold; padding: 5px; }");
    previewLayout->addWidget(instructionLabel);
    
    m_previewLabel = new InteractiveImageLabel();
    m_previewLabel->setMinimumSize(600, 450);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_previewLabel->setScaledContents(false);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("QLabel { border: 1px solid black; background-color: white; cursor: crosshair; }");
    connect(m_previewLabel, &InteractiveImageLabel::imageClicked, this, &ParameterTuningWidget::onImageClicked);
    connect(m_previewLabel, &InteractiveImageLabel::imageRightClicked, this, &ParameterTuningWidget::onImageRightClicked);
    previewLayout->addWidget(m_previewLabel);
    
    // Информация о выборе и кнопки управления
    auto* selectionControlsLayout = new QHBoxLayout();
    m_selectionInfoLabel = new QLabel("Маркеры: 0 позитивных (●) | 0 негативных (✕)");
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
    // Обновляем параметры в памяти (без автоматического применения)
    m_currentParams.dp = m_dpSpinBox->value();
    m_currentParams.minDist = m_minDistSpinBox->value();
    m_currentParams.param1 = m_param1SpinBox->value();
    m_currentParams.param2 = m_param2SpinBox->value();
    m_currentParams.minRadius = m_minRadiusSpinBox->value();
    m_currentParams.maxRadius = m_maxRadiusSpinBox->value();

    // Автоматическое применение убрано - теперь только через кнопку "Применить параметры"
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

    // Рисуем выбранные пользователем клетки (красные точки)
    drawSelectedCells(preview);
    // Рисуем негативные маркеры (красные кресты)
    drawNegativeCells(preview);

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

    // Рисуем позитивные маркеры (красные точки) поверх обнаруженных кругов
    drawSelectedCells(preview);
    // Рисуем негативные маркеры (красные кресты) поверх обнаруженных кругов
    drawNegativeCells(preview);

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
    QString currentPresetName = m_presetCombo->currentText();
    
    // Проверяем, изменился ли текущий пресет (кроме "По умолчанию")
    if (currentPresetName != "По умолчанию" && m_presets.contains(currentPresetName)) {
        // Проверяем, отличаются ли текущие параметры от сохраненных в пресете
        const PresetData& savedPreset = m_presets[currentPresetName];
        bool parametersChanged = (
            savedPreset.params.dp != m_currentParams.dp ||
            savedPreset.params.minDist != m_currentParams.minDist ||
            savedPreset.params.param1 != m_currentParams.param1 ||
            savedPreset.params.param2 != m_currentParams.param2 ||
            savedPreset.params.minRadius != m_currentParams.minRadius ||
            savedPreset.params.maxRadius != m_currentParams.maxRadius ||
            savedPreset.coefficient != SettingsManager::instance().getNmPerPixel()
        );
        
        if (parametersChanged) {
            // Показываем умный диалог
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Сохранение параметров");
            msgBox.setText(QString("Вы изменили параметры набора '%1'.").arg(currentPresetName));
            msgBox.setInformativeText("Что вы хотите сделать?");
            
            QPushButton* updateButton = msgBox.addButton("Да, обновить", QMessageBox::AcceptRole);
            QPushButton* noButton = msgBox.addButton("Нет, не сохранять", QMessageBox::RejectRole);
            QPushButton* newButton = msgBox.addButton("Создать новый", QMessageBox::ActionRole);
            
            msgBox.setDefaultButton(updateButton);
            msgBox.exec();
            
            if (msgBox.clickedButton() == updateButton) {
                // Обновляем текущий пресет
                double currentCoeff = SettingsManager::instance().getNmPerPixel();
                m_presets[currentPresetName] = PresetData(m_currentParams, currentCoeff);
                savePresets();
                LOG_INFO(QString("Пресет '%1' обновлен").arg(currentPresetName));
                return;
            } else if (msgBox.clickedButton() == noButton) {
                // Не сохраняем
                return;
            }
            // Если выбран "Создать новый", продолжаем с вводом имени
        }
    }
    
    // Диалог для ввода имени нового пресета
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
    QJsonObject presets = SettingsManager::instance().getPresets();

    for (auto it = presets.begin(); it != presets.end(); ++it) {
        QString name = it.key();
        QJsonObject presetData = it.value().toObject();

        HoughParams params;
        params.dp = presetData.value("dp").toDouble(1.0);
        params.minDist = presetData.value("minDist").toDouble(30.0);
        params.param1 = presetData.value("param1").toDouble(80.0);
        params.param2 = presetData.value("param2").toDouble(40.0);
        params.minRadius = presetData.value("minRadius").toInt(30);
        params.maxRadius = presetData.value("maxRadius").toInt(130);

        double coefficient = presetData.value("coefficient").toDouble(0.0);
        m_presets[name] = PresetData(params, coefficient);
        m_presetCombo->addItem(name);
    }

    // Загружаем последний выбранный пресет (БЕЗ автоматического применения)
    QString lastPreset = SettingsManager::instance().getLastSelectedPreset();
    int index = m_presetCombo->findText(lastPreset);
    if (index != -1) {
        // Блокируем сигналы, чтобы не вызвался onLoadPreset()
        m_presetCombo->blockSignals(true);
        m_presetCombo->setCurrentIndex(index);
        m_presetCombo->blockSignals(false);

        // Загружаем параметры в UI без применения
        if (lastPreset == "По умолчанию") {
            m_currentParams = HoughParams();
        } else if (m_presets.contains(lastPreset)) {
            const PresetData& presetData = m_presets[lastPreset];
            m_currentParams = presetData.params;

            // Если в пресете есть коэффициент, применяем его
            if (presetData.coefficient > 0.0) {
                SettingsManager::instance().setNmPerPixel(presetData.coefficient);
            }
        }

        // Обновляем значения в спинбоксах (без применения)
        m_dpSpinBox->blockSignals(true);
        m_minDistSpinBox->blockSignals(true);
        m_param1SpinBox->blockSignals(true);
        m_param2SpinBox->blockSignals(true);
        m_minRadiusSpinBox->blockSignals(true);
        m_maxRadiusSpinBox->blockSignals(true);

        m_dpSpinBox->setValue(m_currentParams.dp);
        m_minDistSpinBox->setValue(m_currentParams.minDist);
        m_param1SpinBox->setValue(m_currentParams.param1);
        m_param2SpinBox->setValue(m_currentParams.param2);
        m_minRadiusSpinBox->setValue(m_currentParams.minRadius);
        m_maxRadiusSpinBox->setValue(m_currentParams.maxRadius);

        m_dpSpinBox->blockSignals(false);
        m_minDistSpinBox->blockSignals(false);
        m_param1SpinBox->blockSignals(false);
        m_param2SpinBox->blockSignals(false);
        m_minRadiusSpinBox->blockSignals(false);
        m_maxRadiusSpinBox->blockSignals(false);

        LOG_INFO(QString("Пресет '%1' загружен (без автоматического применения)").arg(lastPreset));
    }
}

void ParameterTuningWidget::savePresets() {
    QJsonObject presets;

    for (auto it = m_presets.begin(); it != m_presets.end(); ++it) {
        QJsonObject presetData;
        presetData["dp"] = it.value().params.dp;
        presetData["minDist"] = it.value().params.minDist;
        presetData["param1"] = it.value().params.param1;
        presetData["param2"] = it.value().params.param2;
        presetData["minRadius"] = it.value().params.minRadius;
        presetData["maxRadius"] = it.value().params.maxRadius;
        presetData["coefficient"] = it.value().coefficient;

        presets[it.key()] = presetData;
    }

    SettingsManager::instance().setPresets(presets);

    // Сохраняем последний выбранный пресет
    SettingsManager::instance().setLastSelectedPreset(m_presetCombo->currentText());
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
    m_selectionInfoLabel->setText(QString("Маркеры: %1 позитивных (●) | %2 негативных (✕)")
        .arg(m_selectedCells.size()).arg(m_negativeCells.size()));
    m_autoFitButton->setEnabled(!m_selectedCells.empty());

    // Обновляем изображение
    if (m_parametersApplied) {
        updatePreview();
    } else {
        showOriginalImage();
    }
}

void ParameterTuningWidget::onImageRightClicked(QPoint position) {
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
    auto it = std::find_if(m_negativeCells.begin(), m_negativeCells.end(),
        [&imagePos](const cv::Point& p) {
            return std::abs(p.x - imagePos.x()) < 20 && std::abs(p.y - imagePos.y()) < 20;
        });

    if (it != m_negativeCells.end()) {
        // Удаляем негативный маркер, если он уже выбран
        m_negativeCells.erase(it);
    } else {
        // Добавляем новый негативный маркер
        m_negativeCells.push_back(cv::Point(imagePos.x(), imagePos.y()));
    }

    // Обновляем информацию
    m_selectionInfoLabel->setText(QString("Маркеры: %1 позитивных (●) | %2 негативных (✕)")
        .arg(m_selectedCells.size()).arg(m_negativeCells.size()));

    // Обновляем изображение
    if (m_parametersApplied) {
        updatePreview();
    } else {
        showOriginalImage();
    }
}

void ParameterTuningWidget::onClearSelection() {
    m_selectedCells.clear();
    m_negativeCells.clear();
    m_selectionInfoLabel->setText("Маркеры: 0 позитивных (●) | 0 негативных (✕)");
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
    m_negativeCells.clear();
    m_selectionInfoLabel->setText("Маркеры: 0 позитивных (●) | 0 негативных (✕)");
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

void ParameterTuningWidget::drawNegativeCells(cv::Mat& image) {
    for (const auto& cell : m_negativeCells) {
        // Рисуем красный крест (две линии пересекающиеся в точке)
        int crossSize = 12; // Размер креста
        int thickness = 3;  // Толщина линий

        // Диагональ от верхнего-левого к нижнему-правому
        cv::line(image,
                 cv::Point(cell.x - crossSize, cell.y - crossSize),
                 cv::Point(cell.x + crossSize, cell.y + crossSize),
                 cv::Scalar(0, 0, 255), thickness);

        // Диагональ от верхнего-правого к нижнему-левому
        cv::line(image,
                 cv::Point(cell.x + crossSize, cell.y - crossSize),
                 cv::Point(cell.x - crossSize, cell.y + crossSize),
                 cv::Scalar(0, 0, 255), thickness);

        // Добавляем белую окантовку для лучшей видимости
        cv::line(image,
                 cv::Point(cell.x - crossSize, cell.y - crossSize),
                 cv::Point(cell.x + crossSize, cell.y + crossSize),
                 cv::Scalar(255, 255, 255), thickness + 2);

        cv::line(image,
                 cv::Point(cell.x + crossSize, cell.y - crossSize),
                 cv::Point(cell.x - crossSize, cell.y + crossSize),
                 cv::Scalar(255, 255, 255), thickness + 2);

        // Перерисовываем крест поверх окантовки
        cv::line(image,
                 cv::Point(cell.x - crossSize, cell.y - crossSize),
                 cv::Point(cell.x + crossSize, cell.y + crossSize),
                 cv::Scalar(0, 0, 255), thickness);

        cv::line(image,
                 cv::Point(cell.x + crossSize, cell.y - crossSize),
                 cv::Point(cell.x - crossSize, cell.y + crossSize),
                 cv::Scalar(0, 0, 255), thickness);
    }
}

void ParameterTuningWidget::optimizeParametersForSelectedCells() {
    if (m_selectedCells.empty()) {
        QMessageBox::information(this, "Информация", "Сначала выберите клетки на изображении.");
        return;
    }

    // Очищаем кэш перед новым запуском оптимизации
    m_circlesCache.clear();
    LOG_INFO("Кэш результатов HoughCircles очищен");

    // Создаем прогресс-диалог
    QProgressDialog* progress = new QProgressDialog(
        "Инициализация оптимизации...",
        "Отмена",
        0, 100,
        this
    );
    progress->setWindowTitle("Автоматический подбор параметров");
    progress->setWindowModality(Qt::WindowModal);
    progress->setMinimumDuration(0); // Показывать сразу
    progress->setMinimumWidth(600); // Увеличиваем ширину для длинных текстов
    progress->setValue(0);
    progress->show();

    QApplication::processEvents(); // Обработать события UI

    auto startTime = std::chrono::steady_clock::now();

    // Запускаем трехфазный алгоритм оптимизации (передаем прогресс-диалог)
    HoughParams bestParams = findBestParametersForCells(m_selectedCells, m_negativeCells, progress);

    auto endTime = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);

    // Закрываем прогресс-диалог
    progress->setValue(100);
    delete progress;

    // Применяем найденные параметры
    m_currentParams = bestParams;

    // Блокируем сигналы на время обновления всех значений
    m_dpSpinBox->blockSignals(true);
    m_minDistSpinBox->blockSignals(true);
    m_param1SpinBox->blockSignals(true);
    m_param2SpinBox->blockSignals(true);
    m_minRadiusSpinBox->blockSignals(true);
    m_maxRadiusSpinBox->blockSignals(true);

    // Обновляем UI
    m_dpSpinBox->setValue(m_currentParams.dp);
    m_minDistSpinBox->setValue(m_currentParams.minDist);
    m_param1SpinBox->setValue(m_currentParams.param1);
    m_param2SpinBox->setValue(m_currentParams.param2);
    m_minRadiusSpinBox->setValue(m_currentParams.minRadius);
    m_maxRadiusSpinBox->setValue(m_currentParams.maxRadius);

    // Разблокируем сигналы
    m_dpSpinBox->blockSignals(false);
    m_minDistSpinBox->blockSignals(false);
    m_param1SpinBox->blockSignals(false);
    m_param2SpinBox->blockSignals(false);
    m_minRadiusSpinBox->blockSignals(false);
    m_maxRadiusSpinBox->blockSignals(false);

    // Автоматически применяем параметры
    m_parametersApplied = true;
    m_confirmButton->setEnabled(true);
    updatePreview();

    // Получаем финальную оценку для отчета
    EvaluationResult finalResult = evaluateParametersAdvanced(bestParams, m_selectedCells, m_negativeCells);

    // Показываем результат пользователю
    QString resultMessage = QString(
        "Оптимизация завершена за %1 сек.\n\n"
        "Результаты:\n"
        "• Покрыто позитивных маркеров: %2/%3 (%4%)\n"
        "• Найдено кругов: %5\n"
        "• Лишних кругов: %6\n"
        "• Нарушений негативных маркеров: %7\n"
        "• Итоговая оценка: %8\n\n"
        "Параметры:\n"
        "• dp = %9\n"
        "• param1 = %10\n"
        "• param2 = %11\n\n"
        "Проверьте результат на превью."
    )
    .arg(elapsed.count())
    .arg(finalResult.matchedCells).arg(m_selectedCells.size())
    .arg(finalResult.coverageRatio * 100.0, 0, 'f', 1)
    .arg(finalResult.totalCircles)
    .arg(finalResult.excessCircles)
    .arg(finalResult.negativeViolations)
    .arg(finalResult.score, 0, 'f', 2)
    .arg(bestParams.dp, 0, 'f', 1)
    .arg(bestParams.param1, 0, 'f', 0)
    .arg(bestParams.param2, 0, 'f', 0);

    if (finalResult.coverageRatio >= 0.99 && finalResult.negativeViolations == 0) {
        QMessageBox::information(this, "Отличный результат!", resultMessage);
    } else if (finalResult.coverageRatio >= 0.7) {
        QMessageBox::information(this, "Хороший результат", resultMessage);
    } else {
        QMessageBox::warning(this, "Результат требует улучшения",
            resultMessage + "\n\nПопробуйте:\n"
            "• Добавить больше позитивных маркеров\n"
            "• Использовать негативные маркеры для исключения артефактов\n"
            "• Скорректировать параметры вручную");
    }

    // Логируем статистику кэша
    LOG_INFO(QString("Статистика кэша: сохранено %1 уникальных результатов").arg(m_circlesCache.size()));
}

ParameterTuningWidget::HoughParams ParameterTuningWidget::findBestParametersForCells(
    const std::vector<cv::Point>& selectedCells,
    const std::vector<cv::Point>& negativeCells,
    QProgressDialog* progress) {

    if (selectedCells.empty()) {
        return m_currentParams;
    }

    LOG_INFO("========================================");
    LOG_INFO("ЗАПУСК ТРЕХФАЗНОГО АЛГОРИТМА ОПТИМИЗАЦИИ");
    LOG_INFO("========================================");
    LOG_INFO(QString("Позитивные маркеры: %1, негативные маркеры: %2")
        .arg(selectedCells.size()).arg(negativeCells.size()));
    LOG_INFO(QString("Фиксированные параметры: minDist=%1, minRadius=%2, maxRadius=%3")
        .arg(m_currentParams.minDist).arg(m_currentParams.minRadius).arg(m_currentParams.maxRadius));

    // ФАЗА 1: Грубый поиск (шаг 5)
    HoughParams coarseResult = coarsePhaseSearch(selectedCells, negativeCells, progress);

    // Проверка отмены после фазы 1
    if (progress && progress->wasCanceled()) {
        LOG_INFO("Оптимизация отменена после фазы 1");
        return coarseResult;
    }

    // ФАЗА 2: Локальная оптимизация (шаг 1)
    HoughParams fineResult = finePhaseSearch(coarseResult, selectedCells, negativeCells, progress);

    // Проверка отмены после фазы 2
    if (progress && progress->wasCanceled()) {
        LOG_INFO("Оптимизация отменена после фазы 2");
        return fineResult;
    }

    // ФАЗА 3: Градиентный спуск
    HoughParams finalResult = gradientDescent(fineResult, selectedCells, negativeCells, progress);

    // Финальная оценка
    EvaluationResult evaluation = evaluateParametersAdvanced(finalResult, selectedCells, negativeCells);

    LOG_INFO("========================================");
    LOG_INFO("ОПТИМИЗАЦИЯ ЗАВЕРШЕНА");
    LOG_INFO(QString("Финальные параметры: dp=%1, param1=%2, param2=%3")
        .arg(finalResult.dp, 0, 'f', 1)
        .arg(finalResult.param1, 0, 'f', 0)
        .arg(finalResult.param2, 0, 'f', 0));
    LOG_INFO(QString("Покрыто: %1/%2 (%3%), кругов: %4, лишних: %5, негативных нарушений: %6, score: %7")
        .arg(evaluation.matchedCells).arg(selectedCells.size())
        .arg(evaluation.coverageRatio * 100.0, 0, 'f', 1)
        .arg(evaluation.totalCircles).arg(evaluation.excessCircles)
        .arg(evaluation.negativeViolations)
        .arg(evaluation.score, 0, 'f', 2));
    LOG_INFO("========================================");

    return finalResult;
}

double ParameterTuningWidget::evaluateParametersForCells(const HoughParams& params, const std::vector<cv::Point>& selectedCells, const std::vector<cv::Point>& negativeCells) {
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

    // Проверяем пересечения с негативными маркерами
    int negativeViolations = 0;
    if (!negativeCells.empty()) {
        for (const auto& circle : filteredCircles) {
            cv::Point2f circleCenter(circle[0], circle[1]);
            double circleRadius = circle[2];

            // Проверяем, попадает ли какой-либо негативный маркер внутрь круга
            for (const auto& negCell : negativeCells) {
                cv::Point2f negPoint(negCell.x, negCell.y);
                double distance = cv::norm(negPoint - circleCenter);

                // Если негативный маркер внутри круга - это нарушение
                if (distance <= circleRadius) {
                    negativeViolations++;
                    break; // Достаточно одного нарушения на круг
                }
            }
        }
    }

    // КРИТИЧЕСКИЙ ШТРАФ: если есть пересечения с негативными маркерами
    if (negativeViolations > 0) {
        // Возвращаем очень низкую оценку, пропорциональную количеству нарушений
        double penalty = 0.01 * std::max(0.0, 1.0 - (double(negativeViolations) / double(filteredCircles.size())));
        LOG_INFO(QString("ШТРАФ: %1 кругов пересекаются с негативными маркерами, оценка снижена до %2")
            .arg(negativeViolations).arg(penalty));
        return penalty;
    }

    // Если все точки покрыты - считаем полную оценку
    double baseScore = 1.0; // 100% за полное покрытие

    // Штраф за ложные срабатывания (чем меньше лишних кругов, тем лучше)
    double efficiency = (unusedCircles == 0) ? 1.0 :
                       std::max(0.3, 1.0 - (double(unusedCircles) / double(filteredCircles.size())));

    // Бонус за точное соответствие (количество кругов = количество точек)
    double precisionBonus = (filteredCircles.size() == selectedCells.size()) ? 0.2 : 0.0;

    double finalScore = baseScore * efficiency + precisionBonus;

    LOG_INFO(QString("Оценка параметров: покрыто %1/%2 точек, лишних кругов %3, негативных нарушений %4, итоговый счет %5")
        .arg(correctMatches).arg(selectedCells.size()).arg(unusedCircles).arg(negativeViolations).arg(finalScore));
    
    return finalScore;
}

std::pair<double, int> ParameterTuningWidget::evaluateParametersForCellsWithCount(const HoughParams& params, const std::vector<cv::Point>& selectedCells, const std::vector<cv::Point>& negativeCells) {
    if (selectedCells.empty()) return {0.0, 0};

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
        return {0.0, 0}; // Некорректные параметры
    }

    if (circles.empty()) return {0.0, 0};

    // Применяем фильтр для устранения наложений с учетом minDist
    std::vector<cv::Vec3f> filteredCircles = filterOverlappingCircles(circles, params.minDist);

    if (filteredCircles.empty()) return {0.0, 0};

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

    // Вычисляем покрытие
    double coverage = double(correctMatches) / double(selectedCells.size());

    // Если не все точки покрыты - возвращаем частичную оценку
    if (correctMatches < selectedCells.size()) {
        return {coverage * 0.1, correctMatches}; // Максимум 10% от полной оценки
    }

    // Считаем ложные срабатывания (лишние круги)
    int unusedCircles = 0;
    for (bool used : circleUsed) {
        if (!used) unusedCircles++;
    }

    // Проверяем пересечения с негативными маркерами
    int negativeViolations = 0;
    if (!negativeCells.empty()) {
        for (const auto& circle : filteredCircles) {
            cv::Point2f circleCenter(circle[0], circle[1]);
            double circleRadius = circle[2];

            // Проверяем, попадает ли какой-либо негативный маркер внутрь круга
            for (const auto& negCell : negativeCells) {
                cv::Point2f negPoint(negCell.x, negCell.y);
                double distance = cv::norm(negPoint - circleCenter);

                // Если негативный маркер внутри круга - это нарушение
                if (distance <= circleRadius) {
                    negativeViolations++;
                    break; // Достаточно одного нарушения на круг
                }
            }
        }
    }

    // КРИТИЧЕСКИЙ ШТРАФ: если есть пересечения с негативными маркерами
    if (negativeViolations > 0) {
        // Возвращаем очень низкую оценку, но сохраняем количество совпадений
        double penalty = 0.01 * std::max(0.0, 1.0 - (double(negativeViolations) / double(filteredCircles.size())));
        return {penalty, correctMatches};
    }

    // Если все точки покрыты - считаем полную оценку
    double baseScore = 1.0; // 100% за полное покрытие

    // Штраф за ложные срабатывания (чем меньше лишних кругов, тем лучше)
    double efficiency = (unusedCircles == 0) ? 1.0 :
                       std::max(0.3, 1.0 - (double(unusedCircles) / double(filteredCircles.size())));

    // Бонус за точное соответствие (количество кругов = количество точек)
    double precisionBonus = (filteredCircles.size() == selectedCells.size()) ? 0.2 : 0.0;

    double finalScore = baseScore * efficiency + precisionBonus;

    return {finalScore, correctMatches};
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

// ============================================================================
// НОВЫЕ МЕТОДЫ ДЛЯ УЛУЧШЕННОЙ ОПТИМИЗАЦИИ ПАРАМЕТРОВ
// ============================================================================

bool ParameterTuningWidget::isValidHeuristicCombination(double dp, double param1, double param2) const {
    // Эвристики для отсечения заведомо плохих комбинаций

    // dp должен быть в разумных пределах
    if (dp < 0.5 || dp > 2.0) return false;

    // param2 не может быть больше param1 (базовое требование HoughCircles)
    if (param2 > param1) return false;

    // Слишком низкие пороги приведут к шуму (множество ложных детекций)
    if (param1 < 10.0 && param2 < 5.0) return false;

    // Слишком высокий param1 пропустит реальные клетки
    if (param1 > 200.0) return false;

    // param2 должен быть хотя бы 20% от param1 для адекватной фильтрации
    if (param2 < param1 * 0.2) return false;

    // Крайне низкие значения обоих параметров дают слишком много шума
    if (param1 < 15.0 && param2 < 8.0) return false;

    return true;
}

std::vector<cv::Vec3f> ParameterTuningWidget::detectCirclesWithCache(const HoughParams& params) {
    // Проверяем кэш
    auto it = m_circlesCache.find(params);
    if (it != m_circlesCache.end()) {
        // Кэш попадание!
        return it->second;
    }

    // Кэш промах - запускаем детекцию
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
        // Некорректные параметры - возвращаем пустой результат
        circles.clear();
    }

    // РАННИЙ ВЫХОД: Если найдено слишком много кругов (>600), не тратим время на фильтрацию
    // Это явно шум от слишком низких порогов (на фото не может быть больше 500-600 клеток)
    if (circles.size() > 600) {
        // Сохраняем пустой результат в кэш, чтобы не проверять снова
        m_circlesCache[params] = std::vector<cv::Vec3f>();
        return std::vector<cv::Vec3f>();
    }

    // Применяем фильтрацию перекрывающихся кругов
    std::vector<cv::Vec3f> filteredCircles = filterOverlappingCircles(circles, params.minDist);

    // Сохраняем в кэш
    m_circlesCache[params] = filteredCircles;

    return filteredCircles;
}

ParameterTuningWidget::EvaluationResult ParameterTuningWidget::evaluateParametersAdvanced(
    const HoughParams& params,
    const std::vector<cv::Point>& selectedCells,
    const std::vector<cv::Point>& negativeCells) {

    EvaluationResult result;

    if (selectedCells.empty()) {
        return result; // score = 0.0
    }

    // Получаем круги (с кэшированием)
    std::vector<cv::Vec3f> circles = detectCirclesWithCache(params);

    if (circles.empty()) {
        return result; // score = 0.0
    }

    result.totalCircles = circles.size();

    // === ШАГ 1: Биективное сопоставление (каждая точка -> максимум один круг) ===
    std::vector<bool> cellMatched(selectedCells.size(), false);
    std::vector<bool> circleUsed(circles.size(), false);

    for (size_t i = 0; i < selectedCells.size(); ++i) {
        const auto& selectedCell = selectedCells[i];
        cv::Point2f cellPoint(selectedCell.x, selectedCell.y);

        double bestDistance = std::numeric_limits<double>::max();
        int bestCircleIdx = -1;

        // Ищем ближайший неиспользованный круг
        for (size_t j = 0; j < circles.size(); ++j) {
            if (circleUsed[j]) continue;

            const auto& circle = circles[j];
            cv::Point2f circleCenter(circle[0], circle[1]);
            double circleRadius = circle[2];
            double distance = cv::norm(cellPoint - circleCenter);

            // ВАЖНО: Точка должна быть внутри круга (с запасом 20%)
            if (distance <= circleRadius * 1.2 && distance < bestDistance) {
                bestDistance = distance;
                bestCircleIdx = j;
            }
        }

        if (bestCircleIdx != -1) {
            // Просто засчитываем ближайший круг без дорогой проверки единственности
            // (проверка единственности была O(n*m) и сильно замедляла оптимизацию)
            cellMatched[i] = true;
            circleUsed[bestCircleIdx] = true;
            result.matchedCells++;
        }
    }

    result.coverageRatio = double(result.matchedCells) / double(selectedCells.size());
    result.excessCircles = 0;
    for (bool used : circleUsed) {
        if (!used) result.excessCircles++;
    }

    // === ШАГ 2: Проверка негативных маркеров ===
    result.negativeViolations = 0;
    if (!negativeCells.empty()) {
        for (const auto& circle : circles) {
            cv::Point2f circleCenter(circle[0], circle[1]);
            double circleRadius = circle[2];

            for (const auto& negCell : negativeCells) {
                cv::Point2f negPoint(negCell.x, negCell.y);
                double distance = cv::norm(negPoint - circleCenter);

                // Если негативный маркер внутри круга - нарушение
                if (distance <= circleRadius) {
                    result.negativeViolations++;
                    break; // Один круг - одно нарушение макс
                }
            }
        }
    }

    // === ШАГ 3: Вычисление итоговой оценки (непрерывная штрафная функция) ===

    // Компонент 1: Покрытие позитивных маркеров (квадратичная функция)
    double coverageScore = result.coverageRatio * result.coverageRatio;

    // Компонент 2: Точность (штраф за лишние круги)
    double precisionScore = 0.0;
    if (result.totalCircles > 0) {
        precisionScore = 1.0 / (1.0 + result.excessCircles);
    }

    // Компонент 3: Критический штраф за негативные нарушения
    double negativePenalty = -10.0 * result.negativeViolations;

    // Бонус за идеальное соответствие (100% покрытие + минимум лишних)
    double perfectBonus = 0.0;
    if (result.coverageRatio >= 0.99 && result.excessCircles <= 2) {
        perfectBonus = 0.5;
    }

    // Итоговая оценка
    result.score = coverageScore * 2.0 + precisionScore + negativePenalty + perfectBonus;

    // Логирование отключено для ускорения оптимизации

    return result;
}

// ============================================================================
// ТРЕХФАЗНЫЙ АЛГОРИТМ ОПТИМИЗАЦИИ
// ============================================================================

ParameterTuningWidget::HoughParams ParameterTuningWidget::coarsePhaseSearch(
    const std::vector<cv::Point>& selectedCells,
    const std::vector<cv::Point>& negativeCells,
    QProgressDialog* progress) {

    LOG_INFO("=== ФАЗА 1: Грубый поиск (шаг: dp=0.2, param1/2=10) ===");

    const double userMinDist = m_currentParams.minDist;
    const int userMinRadius = m_currentParams.minRadius;
    const int userMaxRadius = m_currentParams.maxRadius;

    HoughParams bestParams = m_currentParams;
    EvaluationResult bestResult;
    bestResult.score = -1000.0; // Очень низкое начальное значение

    int totalCombinations = 0;
    int validCombinations = 0;

    // Подсчитаем общее количество комбинаций для прогресс-бара
    // Увеличенный шаг: dp=0.2, param1/2=10 для быстрой грубой оценки
    for (double dp = 0.5; dp <= 2.0; dp += 0.2) {
        for (double param1 = 10.0; param1 <= 200.0; param1 += 10.0) {
            for (double param2 = 10.0; param2 <= 200.0; param2 += 10.0) {
                if (isValidHeuristicCombination(dp, param1, param2)) {
                    totalCombinations++;
                }
            }
        }
    }

    LOG_INFO(QString("Будет проверено ~%1 комбинаций в грубой фазе").arg(totalCombinations));

    int tested = 0;

    for (double dp = 0.5; dp <= 2.0; dp += 0.2) {
        for (double param1 = 10.0; param1 <= 200.0; param1 += 10.0) {
            for (double param2 = 10.0; param2 <= 200.0; param2 += 10.0) {
                // Проверка эвристик
                if (!isValidHeuristicCombination(dp, param1, param2)) {
                    continue;
                }

                validCombinations++;

                HoughParams candidate;
                candidate.dp = dp;
                candidate.minDist = userMinDist;
                candidate.param1 = param1;
                candidate.param2 = param2;
                candidate.minRadius = userMinRadius;
                candidate.maxRadius = userMaxRadius;

                EvaluationResult result = evaluateParametersAdvanced(candidate, selectedCells, negativeCells);
                tested++;

                // ФИЛЬТР: Игнорируем результаты с избыточным количеством детекций
                // Если найдено более чем в 5 раз больше кругов ИЛИ больше 600 - это явно шум
                const int maxAllowedCircles = selectedCells.size() * 5;
                if (result.totalCircles > maxAllowedCircles || result.totalCircles > 600) {
                    continue; // Пропускаем этот результат
                }

                // Критерии улучшения:
                // 1. Приоритет: больше покрытых клеток
                // 2. При равном покрытии: меньше лишних кругов
                // 3. При прочих равных: лучший score
                bool isBetter = false;

                if (result.matchedCells > bestResult.matchedCells) {
                    isBetter = true;
                } else if (result.matchedCells == bestResult.matchedCells) {
                    if (result.excessCircles < bestResult.excessCircles) {
                        isBetter = true;
                    } else if (result.excessCircles == bestResult.excessCircles && result.score > bestResult.score) {
                        isBetter = true;
                    }
                }

                if (isBetter) {
                    bestParams = candidate;
                    bestResult = result;

                    LOG_INFO(QString("Улучшение [грубая]: покрыто %1/%2, кругов %3, лишних %4, score=%5 (dp=%6, p1=%7, p2=%8)")
                        .arg(result.matchedCells).arg(selectedCells.size())
                        .arg(result.totalCircles).arg(result.excessCircles)
                        .arg(result.score, 0, 'f', 2)
                        .arg(dp, 0, 'f', 1).arg(param1, 0, 'f', 0).arg(param2, 0, 'f', 0));
                }

                // Обновление прогресс-бара каждые 10 итераций
                if (progress && tested % 10 == 0) {
                    int progressValue = int((double(tested) / double(totalCombinations)) * 33.0); // 0-33%
                    progress->setValue(progressValue);
                    progress->setLabelText(QString("Фаза 1/3: Грубый поиск (%1/%2)")
                        .arg(tested).arg(totalCombinations));

                    // Обрабатываем события UI каждые 10 итераций (не каждую!)
                    QApplication::processEvents();

                    if (progress->wasCanceled()) {
                        LOG_INFO("Оптимизация отменена пользователем");
                        return bestParams;
                    }
                }
            }
        }
    }

    LOG_INFO(QString("Фаза 1 завершена: проверено %1 из %2 валидных комбинаций")
        .arg(tested).arg(validCombinations));
    LOG_INFO(QString("Лучший результат: покрыто %1/%2, кругов %3, лишних %4, score=%5")
        .arg(bestResult.matchedCells).arg(selectedCells.size())
        .arg(bestResult.totalCircles).arg(bestResult.excessCircles)
        .arg(bestResult.score, 0, 'f', 2));

    return bestParams;
}

ParameterTuningWidget::HoughParams ParameterTuningWidget::finePhaseSearch(
    const HoughParams& startParams,
    const std::vector<cv::Point>& selectedCells,
    const std::vector<cv::Point>& negativeCells,
    QProgressDialog* progress) {

    LOG_INFO("=== ФАЗА 2: Локальная оптимизация (шаг: dp=0.1, param1/2=2) ===");

    const double userMinDist = m_currentParams.minDist;
    const int userMinRadius = m_currentParams.minRadius;
    const int userMaxRadius = m_currentParams.maxRadius;

    HoughParams bestParams = startParams;
    EvaluationResult bestResult = evaluateParametersAdvanced(startParams, selectedCells, negativeCells);

    // Локальная окрестность: ±0.5 для dp, ±10 для param1/2
    const double dpMin = std::max(0.5, startParams.dp - 0.5);
    const double dpMax = std::min(2.0, startParams.dp + 0.5);
    const double p1Min = std::max(10.0, startParams.param1 - 10.0);
    const double p1Max = std::min(200.0, startParams.param1 + 10.0);
    const double p2Min = std::max(5.0, startParams.param2 - 10.0);
    const double p2Max = std::min(200.0, startParams.param2 + 10.0);

    int totalCombinations = 0;
    for (double dp = dpMin; dp <= dpMax; dp += 0.1) {
        for (double param1 = p1Min; param1 <= p1Max; param1 += 2.0) {
            for (double param2 = p2Min; param2 <= p2Max; param2 += 2.0) {
                if (isValidHeuristicCombination(dp, param1, param2)) {
                    totalCombinations++;
                }
            }
        }
    }

    LOG_INFO(QString("Будет проверено ~%1 комбинаций в локальной фазе").arg(totalCombinations));

    int tested = 0;

    for (double dp = dpMin; dp <= dpMax; dp += 0.1) {
        for (double param1 = p1Min; param1 <= p1Max; param1 += 2.0) {
            for (double param2 = p2Min; param2 <= p2Max; param2 += 2.0) {
                if (!isValidHeuristicCombination(dp, param1, param2)) {
                    continue;
                }

                HoughParams candidate;
                candidate.dp = dp;
                candidate.minDist = userMinDist;
                candidate.param1 = param1;
                candidate.param2 = param2;
                candidate.minRadius = userMinRadius;
                candidate.maxRadius = userMaxRadius;

                EvaluationResult result = evaluateParametersAdvanced(candidate, selectedCells, negativeCells);
                tested++;

                // ФИЛЬТР: Игнорируем результаты с избыточным количеством детекций
                // Если найдено более чем в 5 раз больше кругов ИЛИ больше 600 - это явно шум
                const int maxAllowedCircles = selectedCells.size() * 5;
                if (result.totalCircles > maxAllowedCircles || result.totalCircles > 600) {
                    continue; // Пропускаем этот результат
                }

                bool isBetter = false;
                if (result.matchedCells > bestResult.matchedCells) {
                    isBetter = true;
                } else if (result.matchedCells == bestResult.matchedCells) {
                    if (result.excessCircles < bestResult.excessCircles) {
                        isBetter = true;
                    } else if (result.excessCircles == bestResult.excessCircles && result.score > bestResult.score) {
                        isBetter = true;
                    }
                }

                if (isBetter) {
                    bestParams = candidate;
                    bestResult = result;

                    LOG_INFO(QString("Улучшение [локальная]: покрыто %1/%2, кругов %3, лишних %4, score=%5 (dp=%6, p1=%7, p2=%8)")
                        .arg(result.matchedCells).arg(selectedCells.size())
                        .arg(result.totalCircles).arg(result.excessCircles)
                        .arg(result.score, 0, 'f', 2)
                        .arg(dp, 0, 'f', 1).arg(param1, 0, 'f', 0).arg(param2, 0, 'f', 0));
                }

                // Обновление прогресс-бара каждые 5 итераций
                if (progress && tested % 5 == 0) {
                    int progressValue = 33 + int((double(tested) / double(totalCombinations)) * 33.0); // 33-66%
                    progress->setValue(progressValue);
                    progress->setLabelText(QString("Фаза 2/3: Локальная оптимизация (%1/%2)")
                        .arg(tested).arg(totalCombinations));

                    // Обрабатываем события UI каждые 5 итераций
                    QApplication::processEvents();

                    if (progress->wasCanceled()) {
                        LOG_INFO("Оптимизация отменена пользователем");
                        return bestParams;
                    }
                }
            }
        }
    }

    LOG_INFO(QString("Фаза 2 завершена: проверено %1 комбинаций").arg(tested));
    LOG_INFO(QString("Лучший результат: покрыто %1/%2, кругов %3, лишних %4, score=%5")
        .arg(bestResult.matchedCells).arg(selectedCells.size())
        .arg(bestResult.totalCircles).arg(bestResult.excessCircles)
        .arg(bestResult.score, 0, 'f', 2));

    return bestParams;
}

ParameterTuningWidget::HoughParams ParameterTuningWidget::gradientDescent(
    const HoughParams& startParams,
    const std::vector<cv::Point>& selectedCells,
    const std::vector<cv::Point>& negativeCells,
    QProgressDialog* progress) {

    LOG_INFO("=== ФАЗА 3: Градиентный спуск ===");

    const double userMinDist = m_currentParams.minDist;
    const int userMinRadius = m_currentParams.minRadius;
    const int userMaxRadius = m_currentParams.maxRadius;

    HoughParams currentParams = startParams;
    EvaluationResult currentResult = evaluateParametersAdvanced(currentParams, selectedCells, negativeCells);

    const int maxIterations = 20;
    const double learningRate = 0.3; // Скорость обучения
    const double epsilon = 0.01; // Минимальное улучшение для продолжения

    for (int iter = 0; iter < maxIterations; ++iter) {
        // Вычисляем численные градиенты
        const double h_dp = 0.1;
        const double h_p1 = 1.0;
        const double h_p2 = 1.0;

        // Градиент по dp
        HoughParams paramsPlusDp = currentParams;
        paramsPlusDp.dp += h_dp;
        if (paramsPlusDp.dp > 2.0) paramsPlusDp.dp = 2.0;
        double scorePlusDp = evaluateParametersAdvanced(paramsPlusDp, selectedCells, negativeCells).score;
        double grad_dp = (scorePlusDp - currentResult.score) / h_dp;

        // Градиент по param1
        HoughParams paramsPlusP1 = currentParams;
        paramsPlusP1.param1 += h_p1;
        if (paramsPlusP1.param1 > 200.0) paramsPlusP1.param1 = 200.0;
        double scorePlusP1 = evaluateParametersAdvanced(paramsPlusP1, selectedCells, negativeCells).score;
        double grad_p1 = (scorePlusP1 - currentResult.score) / h_p1;

        // Градиент по param2
        HoughParams paramsPlusP2 = currentParams;
        paramsPlusP2.param2 += h_p2;
        if (paramsPlusP2.param2 > 200.0) paramsPlusP2.param2 = 200.0;
        double scorePlusP2 = evaluateParametersAdvanced(paramsPlusP2, selectedCells, negativeCells).score;
        double grad_p2 = (scorePlusP2 - currentResult.score) / h_p2;

        // Обновление параметров в направлении градиента
        HoughParams newParams = currentParams;
        newParams.dp += learningRate * grad_dp;
        newParams.param1 += learningRate * grad_p1;
        newParams.param2 += learningRate * grad_p2;

        // Ограничения
        newParams.dp = std::max(0.5, std::min(2.0, newParams.dp));
        newParams.param1 = std::max(10.0, std::min(200.0, newParams.param1));
        newParams.param2 = std::max(5.0, std::min(200.0, newParams.param2));

        // Проверка эвристик
        if (!isValidHeuristicCombination(newParams.dp, newParams.param1, newParams.param2)) {
            LOG_INFO(QString("Итерация %1: комбинация не прошла эвристики, остановка").arg(iter));
            break;
        }

        EvaluationResult newResult = evaluateParametersAdvanced(newParams, selectedCells, negativeCells);

        // ФИЛЬТР: Пропускаем результаты с избыточным количеством детекций
        if (newResult.totalCircles > 600) {
            LOG_INFO(QString("Итерация %1: избыточное количество детекций (%2), остановка").arg(iter).arg(newResult.totalCircles));
            break;
        }

        // Проверка улучшения
        double improvement = newResult.score - currentResult.score;

        if (improvement > epsilon) {
            currentParams = newParams;
            currentResult = newResult;

            LOG_INFO(QString("Градиент итерация %1: улучшение %2 -> %3 (dp=%4, p1=%5, p2=%6)")
                .arg(iter)
                .arg(currentResult.score - improvement, 0, 'f', 3)
                .arg(currentResult.score, 0, 'f', 3)
                .arg(newParams.dp, 0, 'f', 1)
                .arg(newParams.param1, 0, 'f', 0)
                .arg(newParams.param2, 0, 'f', 0));
        } else {
            LOG_INFO(QString("Итерация %1: улучшение %2 < epsilon, сходимость достигнута")
                .arg(iter).arg(improvement, 0, 'f', 3));
            break;
        }

        // Обновление прогресс-бара (градиентный спуск быстрый, обновляем каждую итерацию)
        if (progress) {
            int progressValue = 66 + int((double(iter + 1) / double(maxIterations)) * 34.0); // 66-100%
            progress->setValue(progressValue);
            progress->setLabelText(QString("Фаза 3/3: Градиентный спуск (итерация %1/%2)")
                .arg(iter + 1).arg(maxIterations));

            QApplication::processEvents(); // Обработать события UI

            if (progress->wasCanceled()) {
                LOG_INFO("Оптимизация отменена пользователем");
                return currentParams;
            }
        }
    }

    LOG_INFO(QString("Фаза 3 завершена. Финальный результат: покрыто %1/%2, кругов %3, лишних %4, score=%5")
        .arg(currentResult.matchedCells).arg(selectedCells.size())
        .arg(currentResult.totalCircles).arg(currentResult.excessCircles)
        .arg(currentResult.score, 0, 'f', 2));

    return currentParams;
}

