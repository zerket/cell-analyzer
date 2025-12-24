#include "settingsdialog.h"
#include "settingsmanager.h"
#include "logger.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QRegularExpressionValidator>
#include <QSlider>
#include <cmath>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
    , m_currentHighlightColor(0, 255, 0)  // Default green for all cells
    , m_currentSelectionColor(255, 0, 0)  // Default red for selected cell
{
    setWindowTitle("Настройки");
    setMinimumWidth(500);
    setupUI();
    loadSettings();
}

void SettingsDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    // ============ COEFFICIENT GROUP ============
    QGroupBox* coeffGroup = new QGroupBox("Коэффициент преобразования");
    QFormLayout* coeffLayout = new QFormLayout(coeffGroup);

    m_coefficientEdit = new QLineEdit();
    m_coefficientEdit->setPlaceholderText("0.00000");
    m_coefficientEdit->setMaximumWidth(150);
    m_coefficientEdit->setToolTip("Коэффициент для преобразования пикселей в микрометры (до 5 знаков после запятой)");
    coeffLayout->addRow("Коэфф (мкм/px):", m_coefficientEdit);

    mainLayout->addWidget(coeffGroup);

    // ============ STATISTICS THRESHOLDS GROUP ============
    QGroupBox* statsGroup = new QGroupBox("Пороги статистики");
    QFormLayout* statsLayout = new QFormLayout(statsGroup);

    m_minThresholdEdit = new QLineEdit();
    m_minThresholdEdit->setPlaceholderText("50.0");
    m_minThresholdEdit->setMaximumWidth(150);
    m_minThresholdEdit->setToolTip("Порог для расчёта % клеток МЕНЬШЕ этого значения");
    QRegularExpressionValidator* validator1 = new QRegularExpressionValidator(QRegularExpression("\\d*\\.?\\d+"), this);
    m_minThresholdEdit->setValidator(validator1);
    statsLayout->addRow("% клеток МЕНЬШЕ значения (мкм):", m_minThresholdEdit);

    m_maxThresholdEdit = new QLineEdit();
    m_maxThresholdEdit->setPlaceholderText("100.0");
    m_maxThresholdEdit->setMaximumWidth(150);
    m_maxThresholdEdit->setToolTip("Порог для расчёта % клеток БОЛЬШЕ этого значения");
    QRegularExpressionValidator* validator2 = new QRegularExpressionValidator(QRegularExpression("\\d*\\.?\\d+"), this);
    m_maxThresholdEdit->setValidator(validator2);
    statsLayout->addRow("% клеток БОЛЬШЕ значения (мкм):", m_maxThresholdEdit);

    mainLayout->addWidget(statsGroup);

    // ============ CELL COLORS GROUP ============
    QGroupBox* colorGroup = new QGroupBox("Цвета клеток");
    QVBoxLayout* colorLayout = new QVBoxLayout(colorGroup);

    // --- Highlight color (all cells) ---
    QHBoxLayout* highlightColorLayout = new QHBoxLayout();

    QLabel* highlightLabel = new QLabel("Цвет всех клеток:");
    highlightLabel->setMinimumWidth(150);
    highlightColorLayout->addWidget(highlightLabel);

    m_highlightColorEdit = new QLineEdit();
    m_highlightColorEdit->setPlaceholderText("#00FF00");
    m_highlightColorEdit->setMaximumWidth(100);
    m_highlightColorEdit->setToolTip("Код цвета для обводки всех клеток в формате #RRGGBB");
    QRegularExpressionValidator* highlightValidator = new QRegularExpressionValidator(
        QRegularExpression("#?[0-9A-Fa-f]{0,6}"), this);
    m_highlightColorEdit->setValidator(highlightValidator);
    connect(m_highlightColorEdit, &QLineEdit::textChanged, this, &SettingsDialog::onHighlightColorTextChanged);
    highlightColorLayout->addWidget(m_highlightColorEdit);

    m_highlightColorPickerButton = new QPushButton("🎨 Выбрать");
    m_highlightColorPickerButton->setMaximumWidth(100);
    m_highlightColorPickerButton->setToolTip("Открыть палитру выбора цвета для всех клеток");
    connect(m_highlightColorPickerButton, &QPushButton::clicked, this, &SettingsDialog::onChooseHighlightColorClicked);
    highlightColorLayout->addWidget(m_highlightColorPickerButton);

    m_highlightColorPreview = new QWidget();
    m_highlightColorPreview->setFixedSize(40, 30);
    m_highlightColorPreview->setStyleSheet("QWidget { border: 2px solid #ccc; border-radius: 5px; }");
    highlightColorLayout->addWidget(m_highlightColorPreview);

    highlightColorLayout->addStretch();
    colorLayout->addLayout(highlightColorLayout);

    // --- Selection color (selected cell) ---
    QHBoxLayout* selectionColorLayout = new QHBoxLayout();

    QLabel* selectionLabel = new QLabel("Цвет выбранной:");
    selectionLabel->setMinimumWidth(150);
    selectionColorLayout->addWidget(selectionLabel);

    m_selectionColorEdit = new QLineEdit();
    m_selectionColorEdit->setPlaceholderText("#FF0000");
    m_selectionColorEdit->setMaximumWidth(100);
    m_selectionColorEdit->setToolTip("Код цвета для обводки выбранной клетки в формате #RRGGBB");
    QRegularExpressionValidator* selectionValidator = new QRegularExpressionValidator(
        QRegularExpression("#?[0-9A-Fa-f]{0,6}"), this);
    m_selectionColorEdit->setValidator(selectionValidator);
    connect(m_selectionColorEdit, &QLineEdit::textChanged, this, &SettingsDialog::onSelectionColorTextChanged);
    selectionColorLayout->addWidget(m_selectionColorEdit);

    m_selectionColorPickerButton = new QPushButton("🎨 Выбрать");
    m_selectionColorPickerButton->setMaximumWidth(100);
    m_selectionColorPickerButton->setToolTip("Открыть палитру выбора цвета для выбранной клетки");
    connect(m_selectionColorPickerButton, &QPushButton::clicked, this, &SettingsDialog::onChooseSelectionColorClicked);
    selectionColorLayout->addWidget(m_selectionColorPickerButton);

    m_selectionColorPreview = new QWidget();
    m_selectionColorPreview->setFixedSize(40, 30);
    m_selectionColorPreview->setStyleSheet("QWidget { border: 2px solid #ccc; border-radius: 5px; }");
    selectionColorLayout->addWidget(m_selectionColorPreview);

    selectionColorLayout->addStretch();
    colorLayout->addLayout(selectionColorLayout);

    QLabel* colorNote = new QLabel("💡 Эти цвета используются для обводки клеток на странице верификации");
    colorNote->setStyleSheet("QLabel { color: #666; font-size: 10px; padding: 5px; }");
    colorNote->setWordWrap(true);
    colorLayout->addWidget(colorNote);

    mainLayout->addWidget(colorGroup);

    // ============ FILTERS GROUP ============
    QGroupBox* filtersGroup = new QGroupBox("Фильтры");
    QVBoxLayout* filtersLayout = new QVBoxLayout(filtersGroup);

    m_ignoreBorderCellsCheckbox = new QCheckBox("Игнорировать пограничные клетки");
    m_ignoreBorderCellsCheckbox->setToolTip("Включить фильтр по видимости клетки");
    connect(m_ignoreBorderCellsCheckbox, &QCheckBox::toggled, this, &SettingsDialog::onIgnoreBorderCellsToggled);
    filtersLayout->addWidget(m_ignoreBorderCellsCheckbox);

    // Visibility threshold slider
    QHBoxLayout* visibilityLayout = new QHBoxLayout();

    QLabel* visibilityLabel = new QLabel("Порог видимости (%):");
    visibilityLabel->setMinimumWidth(150);
    visibilityLayout->addWidget(visibilityLabel);

    m_visibilityThresholdSlider = new QSlider(Qt::Horizontal);
    m_visibilityThresholdSlider->setRange(0, 99);
    m_visibilityThresholdSlider->setValue(5);
    m_visibilityThresholdSlider->setToolTip("Минимальный процент видимости клетки (на основе соотношения сторон bbox)");
    connect(m_visibilityThresholdSlider, &QSlider::valueChanged, this, &SettingsDialog::onVisibilityThresholdChanged);
    visibilityLayout->addWidget(m_visibilityThresholdSlider);

    m_visibilityThresholdValueLabel = new QLabel("5%");
    m_visibilityThresholdValueLabel->setMinimumWidth(40);
    m_visibilityThresholdValueLabel->setStyleSheet("QLabel { font-weight: bold; }");
    visibilityLayout->addWidget(m_visibilityThresholdValueLabel);

    visibilityLayout->addStretch();
    filtersLayout->addLayout(visibilityLayout);

    QLabel* filterNote = new QLabel("💡 Клетки с видимостью меньше порога будут скрыты и исключены из статистики");
    filterNote->setStyleSheet("QLabel { color: #666; font-size: 10px; padding: 5px; }");
    filterNote->setWordWrap(true);
    filtersLayout->addWidget(filterNote);

    QLabel* visibilityExplanation = new QLabel(
        "ℹ️ Видимость = min(ширина bbox, высота bbox) / max(ширина bbox, высота bbox) × 100%\n"
        "Примеры: 100×100 = 100%, 50×100 = 50%, 13×134 ≈ 10%");
    visibilityExplanation->setStyleSheet("QLabel { color: #888; font-size: 9px; padding: 5px; font-style: italic; }");
    visibilityExplanation->setWordWrap(true);
    filtersLayout->addWidget(visibilityExplanation);

    // Cell diameter filter
    QHBoxLayout* diameterLayout = new QHBoxLayout();

    QLabel* minDiameterLabel = new QLabel("Мин. диаметр (px):");
    minDiameterLabel->setMinimumWidth(150);
    diameterLayout->addWidget(minDiameterLabel);

    m_minDiameterEdit = new QLineEdit();
    m_minDiameterEdit->setMaximumWidth(80);
    m_minDiameterEdit->setPlaceholderText("30");
    diameterLayout->addWidget(m_minDiameterEdit);

    diameterLayout->addSpacing(20);

    QLabel* maxDiameterLabel = new QLabel("Макс. диаметр (px):");
    diameterLayout->addWidget(maxDiameterLabel);

    m_maxDiameterEdit = new QLineEdit();
    m_maxDiameterEdit->setMaximumWidth(80);
    m_maxDiameterEdit->setPlaceholderText("160");
    diameterLayout->addWidget(m_maxDiameterEdit);

    diameterLayout->addStretch();
    filtersLayout->addLayout(diameterLayout);

    QLabel* diameterNote = new QLabel("💡 Клетки вне этого диапазона будут отфильтрованы при обработке");
    diameterNote->setStyleSheet("QLabel { color: #666; font-size: 10px; padding: 5px; }");
    diameterNote->setWordWrap(true);
    filtersLayout->addWidget(diameterNote);

    mainLayout->addWidget(filtersGroup);

    // ============ BUTTONS ============
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    m_cancelButton = new QPushButton("Отмена");
    m_cancelButton->setMinimumWidth(100);
    m_cancelButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 8px; }");
    connect(m_cancelButton, &QPushButton::clicked, this, &SettingsDialog::onCancelClicked);
    buttonLayout->addWidget(m_cancelButton);

    m_saveButton = new QPushButton("💾 Сохранить");
    m_saveButton->setMinimumWidth(100);
    m_saveButton->setStyleSheet("QPushButton { background-color: #2196F3; color: white; border-radius: 5px; padding: 8px; font-weight: bold; }");
    connect(m_saveButton, &QPushButton::clicked, this, &SettingsDialog::onSaveClicked);
    buttonLayout->addWidget(m_saveButton);

    mainLayout->addLayout(buttonLayout);

    setLayout(mainLayout);
}

void SettingsDialog::loadSettings()
{
    SettingsManager& settings = SettingsManager::instance();

    // Load coefficient
    double coeff = settings.getCoefficient();
    if (coeff > 0) {
        m_coefficientEdit->setText(QString::number(coeff, 'f', 5));
    }

    // Load statistics thresholds
    double minThreshold = settings.getStatisticsMinThreshold();
    double maxThreshold = settings.getStatisticsMaxThreshold();
    m_minThresholdEdit->setText(QString::number(minThreshold, 'f', 1));
    m_maxThresholdEdit->setText(QString::number(maxThreshold, 'f', 1));

    // Load cell highlight color (all cells)
    QString highlightColorStr = settings.getCellHighlightColor();
    if (!highlightColorStr.isEmpty()) {
        m_currentHighlightColor = QColor(highlightColorStr);
        if (m_currentHighlightColor.isValid()) {
            m_highlightColorEdit->setText(highlightColorStr);
            updateHighlightColorPreview();
        }
    }

    // Load cell selection color (selected cell)
    QString selectionColorStr = settings.getCellSelectionColor();
    if (!selectionColorStr.isEmpty()) {
        m_currentSelectionColor = QColor(selectionColorStr);
        if (m_currentSelectionColor.isValid()) {
            m_selectionColorEdit->setText(selectionColorStr);
            updateSelectionColorPreview();
        }
    }

    // Load ignore border cells
    bool ignoreBorder = settings.getIgnoreBorderCells();
    m_ignoreBorderCellsCheckbox->setChecked(ignoreBorder);

    // Load visibility threshold
    int visibilityThreshold = settings.getCellVisibilityThreshold();
    m_visibilityThresholdSlider->setValue(visibilityThreshold);
    m_visibilityThresholdValueLabel->setText(QString("%1%").arg(visibilityThreshold));

    // Enable/disable slider based on checkbox
    m_visibilityThresholdSlider->setEnabled(ignoreBorder);
    m_visibilityThresholdValueLabel->setEnabled(ignoreBorder);

    // Load cell diameter filter
    int minDiameter = settings.getMinCellDiameter();
    int maxDiameter = settings.getMaxCellDiameter();
    m_minDiameterEdit->setText(QString::number(minDiameter));
    m_maxDiameterEdit->setText(QString::number(maxDiameter));

    LOG_INFO("Settings loaded in dialog");
}

void SettingsDialog::saveSettings()
{
    SettingsManager& settings = SettingsManager::instance();

    // Save coefficient (if set)
    QString coeffText = m_coefficientEdit->text().trimmed();
    if (!coeffText.isEmpty()) {
        bool ok;
        double coeff = coeffText.toDouble(&ok);
        if (ok && coeff > 0) {
            // Limit to 5 decimal places
            coeff = std::round(coeff * 100000.0) / 100000.0;
            settings.setCoefficient(coeff);
            LOG_INFO(QString("Coefficient saved: %1").arg(coeff, 0, 'f', 5));
        }
    }

    // Save statistics thresholds
    bool ok1, ok2;
    double minThreshold = m_minThresholdEdit->text().toDouble(&ok1);
    double maxThreshold = m_maxThresholdEdit->text().toDouble(&ok2);

    if (ok1 && minThreshold > 0) {
        settings.setStatisticsMinThreshold(minThreshold);
        LOG_INFO(QString("Min threshold saved: %1").arg(minThreshold));
    }
    if (ok2 && maxThreshold > 0) {
        settings.setStatisticsMaxThreshold(maxThreshold);
        LOG_INFO(QString("Max threshold saved: %1").arg(maxThreshold));
    }

    // Save cell highlight color (all cells)
    QString highlightColorText = m_highlightColorEdit->text().trimmed();
    if (!highlightColorText.isEmpty()) {
        if (!highlightColorText.startsWith('#')) {
            highlightColorText = '#' + highlightColorText;
        }
        QColor highlightColor(highlightColorText);
        if (highlightColor.isValid()) {
            settings.setCellHighlightColor(highlightColorText);
            LOG_INFO(QString("Cell highlight color saved: %1").arg(highlightColorText));
        }
    }

    // Save cell selection color (selected cell)
    QString selectionColorText = m_selectionColorEdit->text().trimmed();
    if (!selectionColorText.isEmpty()) {
        if (!selectionColorText.startsWith('#')) {
            selectionColorText = '#' + selectionColorText;
        }
        QColor selectionColor(selectionColorText);
        if (selectionColor.isValid()) {
            settings.setCellSelectionColor(selectionColorText);
            LOG_INFO(QString("Cell selection color saved: %1").arg(selectionColorText));
        }
    }

    // Save ignore border cells
    bool ignoreBorder = m_ignoreBorderCellsCheckbox->isChecked();
    settings.setIgnoreBorderCells(ignoreBorder);
    LOG_INFO(QString("Ignore border cells: %1").arg(ignoreBorder));

    // Save visibility threshold
    int visibilityThreshold = m_visibilityThresholdSlider->value();
    settings.setCellVisibilityThreshold(visibilityThreshold);
    LOG_INFO(QString("Visibility threshold: %1%").arg(visibilityThreshold));

    // Save cell diameter filter
    bool okMin, okMax;
    int minDiameter = m_minDiameterEdit->text().toInt(&okMin);
    int maxDiameter = m_maxDiameterEdit->text().toInt(&okMax);
    if (okMin && minDiameter > 0) {
        settings.setMinCellDiameter(minDiameter);
        LOG_INFO(QString("Min cell diameter: %1 px").arg(minDiameter));
    }
    if (okMax && maxDiameter > 0) {
        settings.setMaxCellDiameter(maxDiameter);
        LOG_INFO(QString("Max cell diameter: %1 px").arg(maxDiameter));
    }

    LOG_INFO("All settings saved");
}

bool SettingsDialog::validateInput()
{
    // Validate coefficient (optional)
    QString coeffText = m_coefficientEdit->text().trimmed();
    if (!coeffText.isEmpty()) {
        bool ok;
        double coeff = coeffText.toDouble(&ok);
        if (!ok || coeff <= 0) {
            QMessageBox::warning(this, "Ошибка валидации",
                "Коэффициент должен быть положительным числом.");
            m_coefficientEdit->setFocus();
            return false;
        }
    }

    // Validate min threshold
    QString minText = m_minThresholdEdit->text().trimmed();
    if (minText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Введите минимальный порог статистики.");
        m_minThresholdEdit->setFocus();
        return false;
    }
    bool ok;
    double minThreshold = minText.toDouble(&ok);
    if (!ok || minThreshold <= 0) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Минимальный порог должен быть положительным числом.");
        m_minThresholdEdit->setFocus();
        return false;
    }

    // Validate max threshold
    QString maxText = m_maxThresholdEdit->text().trimmed();
    if (maxText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Введите максимальный порог статистики.");
        m_maxThresholdEdit->setFocus();
        return false;
    }
    double maxThreshold = maxText.toDouble(&ok);
    if (!ok || maxThreshold <= 0) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Максимальный порог должен быть положительным числом.");
        m_maxThresholdEdit->setFocus();
        return false;
    }

    // Check that min < max
    if (minThreshold >= maxThreshold) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Минимальный порог должен быть меньше максимального.");
        m_minThresholdEdit->setFocus();
        return false;
    }

    // Validate highlight color (all cells)
    QString highlightColorText = m_highlightColorEdit->text().trimmed();
    if (highlightColorText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Введите код цвета для всех клеток.");
        m_highlightColorEdit->setFocus();
        return false;
    }
    if (!highlightColorText.startsWith('#')) {
        highlightColorText = '#' + highlightColorText;
    }
    QColor highlightColor(highlightColorText);
    if (!highlightColor.isValid()) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Неверный формат цвета для всех клеток. Используйте формат #RRGGBB (например, #00FF00).");
        m_highlightColorEdit->setFocus();
        return false;
    }

    // Validate selection color (selected cell)
    QString selectionColorText = m_selectionColorEdit->text().trimmed();
    if (selectionColorText.isEmpty()) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Введите код цвета для выбранной клетки.");
        m_selectionColorEdit->setFocus();
        return false;
    }
    if (!selectionColorText.startsWith('#')) {
        selectionColorText = '#' + selectionColorText;
    }
    QColor selectionColor(selectionColorText);
    if (!selectionColor.isValid()) {
        QMessageBox::warning(this, "Ошибка валидации",
            "Неверный формат цвета для выбранной клетки. Используйте формат #RRGGBB (например, #FF0000).");
        m_selectionColorEdit->setFocus();
        return false;
    }

    return true;
}

void SettingsDialog::onSaveClicked()
{
    if (!validateInput()) {
        return;
    }

    saveSettings();

    QMessageBox::information(this, "Успешно", "Настройки сохранены.\nПерезапустите анализ для применения изменений.");
    accept();
}

void SettingsDialog::onCancelClicked()
{
    reject();
}

void SettingsDialog::onChooseHighlightColorClicked()
{
    QColor color = QColorDialog::getColor(m_currentHighlightColor, this, "Выберите цвет для всех клеток");

    if (color.isValid()) {
        m_currentHighlightColor = color;
        m_highlightColorEdit->setText(color.name().toUpper());
        updateHighlightColorPreview();
    }
}

void SettingsDialog::onChooseSelectionColorClicked()
{
    QColor color = QColorDialog::getColor(m_currentSelectionColor, this, "Выберите цвет для выбранной клетки");

    if (color.isValid()) {
        m_currentSelectionColor = color;
        m_selectionColorEdit->setText(color.name().toUpper());
        updateSelectionColorPreview();
    }
}

void SettingsDialog::onHighlightColorTextChanged(const QString& text)
{
    QString colorText = text.trimmed();
    if (!colorText.startsWith('#') && colorText.length() > 0) {
        colorText = '#' + colorText;
    }

    QColor color(colorText);
    if (color.isValid() && colorText.length() >= 7) {
        m_currentHighlightColor = color;
        updateHighlightColorPreview();
    }
}

void SettingsDialog::onSelectionColorTextChanged(const QString& text)
{
    QString colorText = text.trimmed();
    if (!colorText.startsWith('#') && colorText.length() > 0) {
        colorText = '#' + colorText;
    }

    QColor color(colorText);
    if (color.isValid() && colorText.length() >= 7) {
        m_currentSelectionColor = color;
        updateSelectionColorPreview();
    }
}

void SettingsDialog::updateHighlightColorPreview()
{
    m_highlightColorPreview->setStyleSheet(QString(
        "QWidget { background-color: %1; border: 2px solid #ccc; border-radius: 5px; }")
        .arg(m_currentHighlightColor.name()));
}

void SettingsDialog::updateSelectionColorPreview()
{
    m_selectionColorPreview->setStyleSheet(QString(
        "QWidget { background-color: %1; border: 2px solid #ccc; border-radius: 5px; }")
        .arg(m_currentSelectionColor.name()));
}

void SettingsDialog::onIgnoreBorderCellsToggled(bool checked)
{
    // Enable/disable slider based on checkbox state
    m_visibilityThresholdSlider->setEnabled(checked);
    m_visibilityThresholdValueLabel->setEnabled(checked);
}

void SettingsDialog::onVisibilityThresholdChanged(int value)
{
    // Update label to show current value
    m_visibilityThresholdValueLabel->setText(QString("%1%").arg(value));
}
