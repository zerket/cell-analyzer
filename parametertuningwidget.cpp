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
#include "settingsmanager.h"

ParameterTuningWidget::ParameterTuningWidget(const QString& imagePath, QWidget *parent)
    : QWidget(parent), m_imagePath(imagePath) {
    
    m_originalImage = cv::imread(imagePath.toStdString());
    if (m_originalImage.empty()) {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить изображение");
        return;
    }
    
    cv::cvtColor(m_originalImage, m_grayImage, cv::COLOR_BGR2GRAY);
    cv::medianBlur(m_grayImage, m_blurredImage, 5);
    
    // Загружаем параметры из настроек
    m_currentParams = SettingsManager::instance().getHoughParams();
    
    setupUI();
    loadPresets();
    updatePreview();
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
    m_previewLabel = new QLabel();
    m_previewLabel->setMinimumSize(600, 450);
    m_previewLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_previewLabel->setScaledContents(false);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setStyleSheet("QLabel { border: 1px solid black; background-color: white; }");
    previewLayout->addWidget(m_previewLabel);
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
    
    auto* savePresetBtn = new QPushButton("Сохранить текущий набор");
    connect(savePresetBtn, &QPushButton::clicked, this, &ParameterTuningWidget::onSavePreset);
    presetLayout->addWidget(savePresetBtn);
    
    paramsVLayout->addWidget(presetGroup);
    
    // Кнопка подтверждения
    m_confirmButton = new QPushButton("Применить параметры и продолжить");
    m_confirmButton->setMinimumHeight(40);
    m_confirmButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }"
        "QPushButton:hover { background-color: #45a049; }"
    );
    connect(m_confirmButton, &QPushButton::clicked, this, &ParameterTuningWidget::onConfirmClicked);
    paramsVLayout->addWidget(m_confirmButton);
    
    paramsVLayout->addStretch();
    
    contentLayout->addWidget(paramsContainer, 1);
    mainLayout->addLayout(contentLayout);
    
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
    m_currentParams.dp = m_dpSpinBox->value();
    m_currentParams.minDist = m_minDistSpinBox->value();
    m_currentParams.param1 = m_param1SpinBox->value();
    m_currentParams.param2 = m_param2SpinBox->value();
    m_currentParams.minRadius = m_minRadiusSpinBox->value();
    m_currentParams.maxRadius = m_maxRadiusSpinBox->value();
    
    updatePreview();
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
    
    // Рисуем найденные круги
    for (const auto& c : circles) {
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
    std::string info = "Circles found: " + std::to_string(circles.size());
    cv::putText(preview, info, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 
                1.0, cv::Scalar(0, 255, 0), 2);
    
    // Конвертируем в QPixmap и отображаем
    cv::Mat rgb;
    cv::cvtColor(preview, rgb, cv::COLOR_BGR2RGB);
    QImage img = QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
    QPixmap pixmap = QPixmap::fromImage(img);
    
    // Масштабируем изображение, если оно слишком большое
    int maxWidth = m_previewLabel->width() - 10;
    int maxHeight = m_previewLabel->height() - 10;
    if (maxWidth > 0 && maxHeight > 0 && 
        (pixmap.width() > maxWidth || pixmap.height() > maxHeight)) {
        pixmap = pixmap.scaled(maxWidth, maxHeight, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    m_previewLabel->setPixmap(pixmap);
}

void ParameterTuningWidget::onConfirmClicked() {
    emit parametersConfirmed(m_currentParams);
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
        
        m_presets[name] = m_currentParams;
        m_presetCombo->addItem(name);
        m_presetCombo->setCurrentText(name);
        savePresets();
    }
}

void ParameterTuningWidget::onLoadPreset() {
    QString currentText = m_presetCombo->currentText();
    if (currentText == "По умолчанию") {
        m_currentParams = HoughParams();
    } else if (m_presets.contains(currentText)) {
        m_currentParams = m_presets[currentText];
    }
    
    // Обновляем значения в спинбоксах
    m_dpSpinBox->setValue(m_currentParams.dp);
    m_minDistSpinBox->setValue(m_currentParams.minDist);
    m_param1SpinBox->setValue(m_currentParams.param1);
    m_param2SpinBox->setValue(m_currentParams.param2);
    m_minRadiusSpinBox->setValue(m_currentParams.minRadius);
    m_maxRadiusSpinBox->setValue(m_currentParams.maxRadius);
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
        m_presets[name] = params;
        m_presetCombo->addItem(name);
    }
    settings.endArray();
}

void ParameterTuningWidget::savePresets() {
    QSettings settings("CellAnalyzer", "HoughParams");
    settings.beginWriteArray("presets");
    int i = 0;
    for (auto it = m_presets.begin(); it != m_presets.end(); ++it, ++i) {
        settings.setArrayIndex(i);
        settings.setValue("name", it.key());
        settings.setValue("dp", it.value().dp);
        settings.setValue("minDist", it.value().minDist);
        settings.setValue("param1", it.value().param1);
        settings.setValue("param2", it.value().param2);
        settings.setValue("minRadius", it.value().minRadius);
        settings.setValue("maxRadius", it.value().maxRadius);
    }
    settings.endArray();
}