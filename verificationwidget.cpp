// verificationwidget.cpp - NEW DESIGN VERSION (Variant 5)
#include "verificationwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QMessageBox>
#include <QDir>
#include <QStandardPaths>
#include <QTextStream>
#include <QDateTime>
#include <QSettings>
#include <QImage>
#include <QScrollBar>
#include <cmath>
#include "logger.h"
#include "settingsmanager.h"
#include "utils.h"

VerificationWidget::VerificationWidget(const QVector<Cell>& cells, QWidget *parent)
    : QWidget(parent)
    , m_fileTabWidget(nullptr)
    , m_mainSplitter(nullptr)
    , m_cellListScrollArea(nullptr)
    , m_cellListContainer(nullptr)
    , m_cellListLayout(nullptr)
    , m_previewWidget(nullptr)
    , m_infoPanel(nullptr)
    , m_cellNumberLabel(nullptr)
    , m_cellPositionLabel(nullptr)
    , m_cellRadiusLabel(nullptr)
    , m_cellAreaLabel(nullptr)
    , m_coefficientEdit(nullptr)
    , m_editCoefficientButton(nullptr)
    , m_recalcButton(nullptr)
    , m_clearDiametersButton(nullptr)
    , m_statisticsButton(nullptr)
    , m_saveButton(nullptr)
    , m_finishButton(nullptr)
    , m_cells(cells)
    , m_selectedCellIndex(-1)
    , m_thumbnailLoadTimer(nullptr)
    , m_thumbnailLoadIndex(0)
{
    LOG_INFO("VerificationWidget constructor called (New Design)");
    LOG_INFO(QString("Received %1 cells").arg(cells.size()));

    try {
        groupCellsByFile();
        LOG_INFO("groupCellsByFile completed");

        setupUI();
        LOG_INFO("setupUI completed");

        loadSavedCoefficient();
        LOG_INFO("loadSavedCoefficient completed");

        // Select first cell by default
        if (!m_cells.isEmpty() && !m_currentFilePath.isEmpty()) {
            selectCell(0);
            LOG_INFO("First cell selected");
        }
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Exception in VerificationWidget constructor: %1").arg(e.what()));
    } catch (...) {
        LOG_ERROR("Unknown exception in VerificationWidget constructor");
    }
}

VerificationWidget::~VerificationWidget()
{
    LOG_INFO("VerificationWidget destructor called");

    // Останавливаем и удаляем таймер
    if (m_thumbnailLoadTimer) {
        m_thumbnailLoadTimer->stop();
        m_thumbnailLoadTimer->deleteLater();
        m_thumbnailLoadTimer = nullptr;
    }
}

void VerificationWidget::groupCellsByFile()
{
    m_cellsByFile.clear();

    for (int i = 0; i < m_cells.size(); ++i) {
        QString imagePath = QString::fromStdString(m_cells[i].imagePath);
        m_cellsByFile[imagePath].append(i);
    }

    LOG_INFO(QString("Cells grouped into %1 files").arg(m_cellsByFile.size()));
}

void VerificationWidget::setupUI()
{
    LOG_INFO("setupUI: Creating main layout");
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);
    mainLayout->setSpacing(5);

    // File tabs at the top
    LOG_INFO("setupUI: Creating file tabs");
    m_fileTabWidget = new QTabWidget(this);
    m_fileTabWidget->setMaximumHeight(35);

    // Block signals while creating tabs to avoid premature onFileTabChanged calls
    m_fileTabWidget->blockSignals(true);

    // Create tabs for each file
    int tabIndex = 0;
    for (auto it = m_cellsByFile.begin(); it != m_cellsByFile.end(); ++it) {
        QString filePath = it.key();
        QVector<int> cellIndices = it.value();

        QString fileName = QFileInfo(filePath).fileName();
        QString tabLabel = QString("%1 (%2)").arg(fileName).arg(cellIndices.size());
        m_fileTabWidget->addTab(new QWidget(), tabLabel);
        m_fileTabWidget->setTabToolTip(tabIndex++, filePath);
        LOG_INFO(QString("setupUI: Added tab for %1 with %2 cells").arg(fileName).arg(cellIndices.size()));
    }

    // Unblock signals AFTER all UI is created
    m_fileTabWidget->blockSignals(false);

    // NOW connect the signal
    connect(m_fileTabWidget, &QTabWidget::currentChanged, this, &VerificationWidget::onFileTabChanged);

    mainLayout->addWidget(m_fileTabWidget);
    LOG_INFO("setupUI: File tabs created");

    // Main splitter (25% left, 75% right)
    LOG_INFO("setupUI: Creating main splitter");
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);

    // LEFT PANEL: Cell list
    LOG_INFO("setupUI: Creating cell list panel");
    m_cellListScrollArea = new QScrollArea(this);
    m_cellListScrollArea->setWidgetResizable(true);
    m_cellListScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    m_cellListContainer = new QWidget();
    m_cellListLayout = new QVBoxLayout(m_cellListContainer);
    m_cellListLayout->setContentsMargins(5, 5, 5, 5);
    m_cellListLayout->setSpacing(5);
    m_cellListLayout->addStretch();

    m_cellListContainer->setLayout(m_cellListLayout);
    m_cellListScrollArea->setWidget(m_cellListContainer);

    m_mainSplitter->addWidget(m_cellListScrollArea);

    // RIGHT PANEL: Preview + Info
    LOG_INFO("setupUI: Creating right panel");
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(5);

    // Preview with markup
    LOG_INFO("setupUI: Creating preview widget");
    m_previewWidget = new MarkupImageWidget(this);
    connect(m_previewWidget, &MarkupImageWidget::cellClicked, this, &VerificationWidget::onImageCellClicked);
    connect(m_previewWidget, &MarkupImageWidget::cellRightClicked, this, &VerificationWidget::onImageCellRightClicked);
    rightLayout->addWidget(m_previewWidget, 1);

    // Zoom controls toolbar
    QWidget* zoomToolbar = new QWidget(this);
    zoomToolbar->setMaximumHeight(50);
    QHBoxLayout* zoomLayout = new QHBoxLayout(zoomToolbar);
    zoomLayout->setContentsMargins(5, 5, 5, 5);
    zoomLayout->setSpacing(5);

    QLabel* zoomLabel = new QLabel("Масштаб:");
    zoomLayout->addWidget(zoomLabel);

    QPushButton* zoomOutBtn = new QPushButton("-");
    zoomOutBtn->setFixedSize(30, 30);
    zoomOutBtn->setToolTip("Уменьшить (Ctrl + колесико вниз)");
    zoomOutBtn->setStyleSheet("QPushButton { font-size: 18px; font-weight: bold; }");
    connect(zoomOutBtn, &QPushButton::clicked, m_previewWidget, &MarkupImageWidget::zoomOut);
    zoomLayout->addWidget(zoomOutBtn);

    QPushButton* zoomInBtn = new QPushButton("+");
    zoomInBtn->setFixedSize(30, 30);
    zoomInBtn->setToolTip("Увеличить (Ctrl + колесико вверх)");
    zoomInBtn->setStyleSheet("QPushButton { font-size: 18px; font-weight: bold; }");
    connect(zoomInBtn, &QPushButton::clicked, m_previewWidget, &MarkupImageWidget::zoomIn);
    zoomLayout->addWidget(zoomInBtn);

    QPushButton* resetZoomBtn = new QPushButton("100%");
    resetZoomBtn->setFixedSize(50, 30);
    resetZoomBtn->setToolTip("Сбросить масштаб");
    connect(resetZoomBtn, &QPushButton::clicked, m_previewWidget, &MarkupImageWidget::resetZoom);
    zoomLayout->addWidget(resetZoomBtn);

    QPushButton* fitToWindowBtn = new QPushButton("По размеру");
    fitToWindowBtn->setToolTip("Подогнать под окно");
    connect(fitToWindowBtn, &QPushButton::clicked, m_previewWidget, &MarkupImageWidget::fitToWindow);
    zoomLayout->addWidget(fitToWindowBtn);

    QLabel* zoomValueLabel = new QLabel("100%");
    zoomValueLabel->setMinimumWidth(60);
    zoomValueLabel->setAlignment(Qt::AlignCenter);
    connect(m_previewWidget, &MarkupImageWidget::zoomChanged, [zoomValueLabel](double zoom) {
        zoomValueLabel->setText(QString("%1%").arg(static_cast<int>(zoom * 100)));
    });
    zoomLayout->addWidget(zoomValueLabel);

    zoomLayout->addStretch();
    zoomToolbar->setLayout(zoomLayout);
    rightLayout->addWidget(zoomToolbar);

    // Cell info panel
    m_infoPanel = new QWidget(this);
    m_infoPanel->setMaximumHeight(100);
    QVBoxLayout* infoLayout = new QVBoxLayout(m_infoPanel);
    infoLayout->setContentsMargins(10, 5, 10, 5);

    QLabel* infoTitle = new QLabel("<b>Информация о клетке:</b>");
    infoLayout->addWidget(infoTitle);

    m_cellNumberLabel = new QLabel("Не выбрано");
    m_cellPositionLabel = new QLabel("Позиция: -");
    m_cellRadiusLabel = new QLabel("Радиус: -");
    m_cellAreaLabel = new QLabel("Видимость: -");

    infoLayout->addWidget(m_cellNumberLabel);
    infoLayout->addWidget(m_cellPositionLabel);
    infoLayout->addWidget(m_cellRadiusLabel);

    m_infoPanel->setLayout(infoLayout);
    rightLayout->addWidget(m_infoPanel);

    rightPanel->setLayout(rightLayout);
    m_mainSplitter->addWidget(rightPanel);

    // Set splitter sizes (25% / 75%)
    m_mainSplitter->setStretchFactor(0, 1);
    m_mainSplitter->setStretchFactor(1, 3);

    // Set initial sizes: 25% left, 75% right (assuming 1200px window width)
    QList<int> sizes;
    sizes << 300 << 900;  // 300px for list, 900px for preview
    m_mainSplitter->setSizes(sizes);

    mainLayout->addWidget(m_mainSplitter, 1);

    // BOTTOM TOOLBAR
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(10);

    // Coefficient section
    QLabel* coeffLabel = new QLabel("Коэфф (мкм/px):");
    bottomLayout->addWidget(coeffLabel);

    m_coefficientEdit = new QLineEdit();
    m_coefficientEdit->setReadOnly(true);
    m_coefficientEdit->setMaximumWidth(120);
    m_coefficientEdit->setPlaceholderText("0.00000");
    m_coefficientEdit->setAlignment(Qt::AlignCenter);
    connect(m_coefficientEdit, &QLineEdit::editingFinished, this, &VerificationWidget::onCoefficientEditingFinished);
    connect(m_coefficientEdit, &QLineEdit::returnPressed, this, &VerificationWidget::onCoefficientEditingFinished);
    bottomLayout->addWidget(m_coefficientEdit);

    // Edit coefficient button (pencil icon)
    m_editCoefficientButton = new QPushButton("✏️");
    m_editCoefficientButton->setFixedSize(30, 30);
    m_editCoefficientButton->setToolTip("Редактировать коэффициент");
    m_editCoefficientButton->setStyleSheet(
        "QPushButton { "
        "border: 1px solid #ccc; "
        "border-radius: 5px; "
        "padding: 2px; "
        "background-color: white; "
        "}"
        "QPushButton:hover { "
        "background-color: #E3F2FD; "
        "border: 1px solid #90CAF9; "
        "}"
    );
    connect(m_editCoefficientButton, &QPushButton::clicked, this, &VerificationWidget::onEditCoefficientClicked);
    bottomLayout->addWidget(m_editCoefficientButton);

    m_recalcButton = new QPushButton("Пересчитать");
    m_recalcButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 5px 15px; }");
    connect(m_recalcButton, &QPushButton::clicked, this, &VerificationWidget::onRecalculateClicked);
    bottomLayout->addWidget(m_recalcButton);

    m_clearDiametersButton = new QPushButton("Очистить");
    m_clearDiametersButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 5px 15px; }");
    connect(m_clearDiametersButton, &QPushButton::clicked, this, &VerificationWidget::onClearDiametersClicked);
    bottomLayout->addWidget(m_clearDiametersButton);

    bottomLayout->addStretch();

    // Statistics button
    m_statisticsButton = new QPushButton("📊 Статистика");
    m_statisticsButton->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; border-radius: 10px; padding: 8px 16px; font-weight: bold; }");
    connect(m_statisticsButton, &QPushButton::clicked, this, &VerificationWidget::statisticsRequested);
    bottomLayout->addWidget(m_statisticsButton);

    bottomLayout->addStretch();

    // Save and finish buttons
    m_saveButton = new QPushButton("💾 Сохранить");
    m_saveButton->setStyleSheet("QPushButton { border: 1px solid #4CAF50; color: #4CAF50; border-radius: 5px; padding: 5px 15px; }");
    connect(m_saveButton, &QPushButton::clicked, this, &VerificationWidget::onSaveCellsClicked);
    bottomLayout->addWidget(m_saveButton);

    m_finishButton = new QPushButton("✓ Завершить");
    m_finishButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; border-radius: 10px; padding: 8px 16px; font-weight: bold; }");
    connect(m_finishButton, &QPushButton::clicked, this, &VerificationWidget::analysisCompleted);
    bottomLayout->addWidget(m_finishButton);

    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);

    // Initialize lazy loading timer
    m_thumbnailLoadTimer = new QTimer(this);
    m_thumbnailLoadTimer->setSingleShot(false);
    m_thumbnailLoadTimer->setInterval(50); // Загружаем по 10 изображений в секунду
    connect(m_thumbnailLoadTimer, &QTimer::timeout, this, &VerificationWidget::loadNextThumbnailBatch);

    // Initialize first tab
    if (m_fileTabWidget->count() > 0) {
        onFileTabChanged(0);
    }

    LOG_INFO("VerificationWidget UI setup completed");
}

void VerificationWidget::onFileTabChanged(int index)
{
    if (index < 0 || index >= m_cellsByFile.size()) return;

    // Останавливаем предыдущий таймер загрузки
    if (m_thumbnailLoadTimer && m_thumbnailLoadTimer->isActive()) {
        m_thumbnailLoadTimer->stop();
    }

    // Get file path for this tab
    QStringList filePaths = m_cellsByFile.keys();
    m_currentFilePath = filePaths[index];

    LOG_INFO(QString("File tab changed to: %1").arg(m_currentFilePath));

    updateCellList();
    updatePreviewImage();

    // Select first cell in this file
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    if (!cellIndices.isEmpty()) {
        selectCell(cellIndices[0]);
    }
}

void VerificationWidget::updateCellList()
{
    // Clear existing widgets
    while (m_cellListLayout->count() > 1) { // Keep the stretch
        QLayoutItem* item = m_cellListLayout->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    m_cellWidgets.clear();

    // Check if current file path is valid
    if (m_currentFilePath.isEmpty() || !m_cellsByFile.contains(m_currentFilePath)) {
        LOG_WARNING("updateCellList: invalid current file path");
        return;
    }

    // Get cells for current file
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];

    // Create cell list items
    for (int i = 0; i < cellIndices.size(); ++i) {
        int globalIndex = cellIndices[i];
        const Cell& cell = m_cells[globalIndex];

        CellListItemWidget* cellWidget = new CellListItemWidget(i + 1, cell, this);

        // Автоматически заполняем диаметр, если коэффициент существует
        double currentCoeff = SettingsManager::instance().getCoefficient();
        if (currentCoeff > 0.0) {
            double calculatedDiameter = cell.diameterPx * currentCoeff;
            cellWidget->setDiameterNm(calculatedDiameter);
        }

        connect(cellWidget, &CellListItemWidget::clicked, this, &VerificationWidget::onCellItemClicked);
        connect(cellWidget, &CellListItemWidget::removeRequested, this, &VerificationWidget::onCellItemRemoved);
        connect(cellWidget, &CellListItemWidget::diameterNmChanged, this, &VerificationWidget::onDiameterNmChanged);

        m_cellListLayout->insertWidget(m_cellListLayout->count() - 1, cellWidget);
        m_cellWidgets.append(cellWidget);
    }

    LOG_INFO(QString("Updated cell list with %1 cells").arg(cellIndices.size()));

    // Начинаем постепенную загрузку изображений
    m_thumbnailLoadIndex = 0;
    if (m_thumbnailLoadTimer && !m_thumbnailLoadTimer->isActive()) {
        m_thumbnailLoadTimer->start();
    }
}

void VerificationWidget::updatePreviewImage()
{
    // Check if current file path is valid
    if (m_currentFilePath.isEmpty() || !m_cellsByFile.contains(m_currentFilePath)) {
        LOG_WARNING("updatePreviewImage: invalid current file path");
        return;
    }

    // Load and display image with all cells
    m_previewWidget->setImage(m_currentFilePath);

    // Get cells for current file
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    QVector<Cell> fileCells;
    for (int idx : cellIndices) {
        if (idx >= 0 && idx < m_cells.size()) {
            fileCells.append(m_cells[idx]);
        } else {
            LOG_ERROR(QString("Invalid cell index: %1").arg(idx));
        }
    }

    m_previewWidget->setCells(fileCells);
}

void VerificationWidget::selectCell(int globalCellIndex)
{
    if (globalCellIndex < 0 || globalCellIndex >= m_cells.size()) {
        LOG_WARNING(QString("selectCell: invalid index %1").arg(globalCellIndex));
        return;
    }

    if (m_currentFilePath.isEmpty() || !m_cellsByFile.contains(m_currentFilePath)) {
        LOG_WARNING("selectCell: invalid current file path");
        return;
    }

    m_selectedCellIndex = globalCellIndex;

    // Update cell list selection
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    int localIndex = cellIndices.indexOf(globalCellIndex);

    for (int i = 0; i < m_cellWidgets.size(); ++i) {
        if (m_cellWidgets[i]) {
            m_cellWidgets[i]->setSelected(i == localIndex);
        }
    }

    // Auto-scroll to selected cell in the list
    if (localIndex >= 0 && localIndex < m_cellWidgets.size() && m_cellWidgets[localIndex]) {
        m_cellListScrollArea->ensureWidgetVisible(m_cellWidgets[localIndex], 0, 50);
        LOG_INFO(QString("Auto-scrolled to cell at local index %1").arg(localIndex));
    }

    // Update preview selection
    if (m_previewWidget) {
        m_previewWidget->setSelectedCell(localIndex);
    }

    // Update info panel
    updateCellInfoPanel();

    // Detailed logging for debugging border cells
    const Cell& cell = m_cells[globalCellIndex];
    LOG_INFO(QString("========================================"));
    LOG_INFO(QString("CELL #%1 CLICKED (Global index: %2, Local index: %3)").arg(localIndex + 1).arg(globalCellIndex).arg(localIndex));
    LOG_INFO(QString("========================================"));
    LOG_INFO(QString("YOLO BBOX (from model):"));
    LOG_INFO(QString("  bbox_x = %1, bbox_y = %2").arg(cell.bbox_x).arg(cell.bbox_y));
    LOG_INFO(QString("  bbox_width = %1, bbox_height = %2").arg(cell.bbox_width).arg(cell.bbox_height));
    LOG_INFO(QString("  bbox corners: (%1, %2) to (%3, %4)")
        .arg(cell.bbox_x)
        .arg(cell.bbox_y)
        .arg(cell.bbox_x + cell.bbox_width)
        .arg(cell.bbox_y + cell.bbox_height));
    LOG_INFO(QString(""));
    LOG_INFO(QString("CIRCLE (calculated from bbox):"));
    LOG_INFO(QString("  center_x = %1, center_y = %2").arg(cell.center_x).arg(cell.center_y));
    LOG_INFO(QString("  radius = %1 px").arg(cell.radius));
    LOG_INFO(QString("  diameter = %1 px").arg(cell.diameter_pixels));
    LOG_INFO(QString(""));
    LOG_INFO(QString("OTHER INFO:"));
    LOG_INFO(QString("  area = %1 px²").arg(cell.area));
    LOG_INFO(QString("  confidence = %1").arg(cell.confidence));
    LOG_INFO(QString("  diameter_um = %1 μm").arg(cell.diameter_um, 0, 'f', 2));
    LOG_INFO(QString("========================================"));
}

void VerificationWidget::updateCellInfoPanel()
{
    if (m_selectedCellIndex < 0 || m_selectedCellIndex >= m_cells.size()) {
        m_cellNumberLabel->setText("Не выбрано");
        m_cellPositionLabel->setText("Позиция: -");
        m_cellRadiusLabel->setText("Радиус: -");
        return;
    }

    const Cell& cell = m_cells[m_selectedCellIndex];

    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    int localIndex = cellIndices.indexOf(m_selectedCellIndex);

    m_cellNumberLabel->setText(QString("<b>Клетка #%1</b>").arg(localIndex + 1));
    m_cellPositionLabel->setText(QString("Позиция: (%1, %2)").arg(cell.circle[0], 0, 'f', 0).arg(cell.circle[1], 0, 'f', 0));
    m_cellRadiusLabel->setText(QString("Радиус: %1 px (диаметр: %2 px)").arg(cell.circle[2], 0, 'f', 1).arg(cell.diameterPx, 0, 'f', 1));
}

void VerificationWidget::onCellItemClicked(CellListItemWidget* item)
{
    if (!item) return;

    // Find global index
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    int localIndex = m_cellWidgets.indexOf(item);
    if (localIndex >= 0 && localIndex < cellIndices.size()) {
        selectCell(cellIndices[localIndex]);
    }
}

void VerificationWidget::onImageCellClicked(int localCellIndex)
{
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    if (localCellIndex >= 0 && localCellIndex < cellIndices.size()) {
        selectCell(cellIndices[localCellIndex]);
    }
}

void VerificationWidget::onImageCellRightClicked(int localCellIndex)
{
    // Удаление клетки по правому клику на изображении
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    if (localCellIndex >= 0 && localCellIndex < cellIndices.size()) {
        int globalIndex = cellIndices[localCellIndex];

        // Remove cell from data
        m_cells.removeAt(globalIndex);

        // Regroup cells
        groupCellsByFile();

        // Update current tab label
        int currentTabIndex = m_fileTabWidget->currentIndex();
        QStringList filePaths = m_cellsByFile.keys();
        if (currentTabIndex < filePaths.size()) {
            QString filePath = filePaths[currentTabIndex];
            QString fileName = QFileInfo(filePath).fileName();
            int count = m_cellsByFile[filePath].size();
            m_fileTabWidget->setTabText(currentTabIndex, QString("%1 (%2)").arg(fileName).arg(count));
        }

        // Refresh UI
        updateCellList();
        updatePreviewImage();

        // Select next cell if available
        cellIndices = m_cellsByFile[m_currentFilePath];
        if (!cellIndices.isEmpty()) {
            int nextIndex = qMin(localCellIndex, cellIndices.size() - 1);
            selectCell(cellIndices[nextIndex]);
        } else {
            m_selectedCellIndex = -1;
            updateCellInfoPanel();
        }

        LOG_INFO(QString("Cell removed by right-click on image: local index %1").arg(localCellIndex));
    }
}

void VerificationWidget::onCellItemRemoved(CellListItemWidget* item)
{
    if (!item) return;

    // Find global index
    QVector<int> cellIndices = m_cellsByFile[m_currentFilePath];
    int localIndex = m_cellWidgets.indexOf(item);
    if (localIndex >= 0 && localIndex < cellIndices.size()) {
        int globalIndex = cellIndices[localIndex];

        // Сохраняем текущую позицию скролла
        int scrollPosition = 0;
        if (m_cellListScrollArea) {
            scrollPosition = m_cellListScrollArea->verticalScrollBar()->value();
        }

        // Remove cell from data
        m_cells.removeAt(globalIndex);

        // Regroup cells
        groupCellsByFile();

        // Update current tab label
        int currentTabIndex = m_fileTabWidget->currentIndex();
        QStringList filePaths = m_cellsByFile.keys();
        if (currentTabIndex < filePaths.size()) {
            QString filePath = filePaths[currentTabIndex];
            QString fileName = QFileInfo(filePath).fileName();
            int count = m_cellsByFile[filePath].size();
            m_fileTabWidget->setTabText(currentTabIndex, QString("%1 (%2)").arg(fileName).arg(count));
        }

        // Refresh UI
        updateCellList();
        updatePreviewImage();

        // Восстанавливаем позицию скролла
        if (m_cellListScrollArea) {
            m_cellListScrollArea->verticalScrollBar()->setValue(scrollPosition);
        }

        // Select next cell if available
        cellIndices = m_cellsByFile[m_currentFilePath];
        if (!cellIndices.isEmpty()) {
            int nextIndex = qMin(localIndex, cellIndices.size() - 1);
            selectCell(cellIndices[nextIndex]);
        } else {
            m_selectedCellIndex = -1;
            updateCellInfoPanel();
        }

        LOG_INFO(QString("Removed cell at index %1").arg(globalIndex));
    }
}

void VerificationWidget::onDiameterNmChanged()
{
    updateRecalcButtonState();
}

void VerificationWidget::onRecalculateClicked()
{
    recalculateDiameters();
}

void VerificationWidget::onClearDiametersClicked()
{
    for (CellListItemWidget* widget : m_cellWidgets) {
        if (widget) {
            widget->clearDiameterNm();
        }
    }

    m_coefficientEdit->clear();
    updateRecalcButtonState();
}

void VerificationWidget::updateRecalcButtonState()
{
    bool anyFilled = false;
    for (CellListItemWidget* widget : m_cellWidgets) {
        if (widget && !widget->diameterNmText().isEmpty()) {
            anyFilled = true;
            break;
        }
    }
    m_recalcButton->setEnabled(anyFilled);
}

void VerificationWidget::recalculateDiameters()
{
    QVector<double> scales;

    // Collect scales from all filled fields
    for (CellListItemWidget* widget : m_cellWidgets) {
        if (!widget) continue;
        QString nmText = widget->diameterNmText();
        if (!nmText.isEmpty()) {
            bool ok;
            double nm = nmText.toDouble(&ok);
            if (ok && nm > 0 && widget->diameterPx() > 0) {
                scales.append(nm / widget->diameterPx());
            }
        }
    }

    if (scales.isEmpty()) {
        QMessageBox::information(this, "Информация",
            "Введите хотя бы одно значение диаметра в микрометрах для расчета коэффициента");
        return;
    }

    // Calculate average scale
    double avgScale = std::accumulate(scales.begin(), scales.end(), 0.0) / scales.size();

    // Apply to empty fields (only in current file)
    for (CellListItemWidget* widget : m_cellWidgets) {
        if (widget && widget->diameterNmText().isEmpty()) {
            double nmValue = widget->diameterPx() * avgScale;
            widget->setDiameterNm(nmValue);
        }
    }

    // Limit to 5 decimal places
    avgScale = std::round(avgScale * 100000.0) / 100000.0;

    // Update coefficient display and save to settings
    m_coefficientEdit->setText(QString::number(avgScale, 'f', 5));
    SettingsManager::instance().setCoefficient(avgScale);

    LOG_INFO(QString("Recalculated with coefficient: %1 μm/px").arg(avgScale, 0, 'f', 5));
}

void VerificationWidget::loadSavedCoefficient()
{
    double savedCoeff = SettingsManager::instance().getCoefficient();
    LOG_INFO(QString("loadSavedCoefficient: savedCoeff=%1").arg(savedCoeff));

    if (savedCoeff > 0) {
        m_coefficientEdit->setText(QString::number(savedCoeff, 'f', 5));
        LOG_INFO(QString("Loaded saved coefficient: %1 μm/px").arg(savedCoeff, 0, 'f', 5));
    }
}

QVector<Cell> VerificationWidget::getVerifiedCells() const
{
    QVector<Cell> updatedCells = m_cells;

    // Получаем коэффициент из настроек
    double currentCoeff = SettingsManager::instance().getCoefficient();

    // Обновляем диаметры для текущего отображаемого файла из виджетов
    QVector<int> currentIndices = m_cellsByFile[m_currentFilePath];
    for (int i = 0; i < m_cellWidgets.size() && i < currentIndices.size(); ++i) {
        if (m_cellWidgets[i]) {
            int globalIndex = currentIndices[i];
            double diameterNm = m_cellWidgets[i]->getDiameterNm();

            // Если поле пустое (0.0) и есть коэффициент, применяем его
            if (diameterNm == 0.0 && currentCoeff > 0.0) {
                diameterNm = updatedCells[globalIndex].diameterPx * currentCoeff;
            }

            updatedCells[globalIndex].diameter_um = diameterNm;
            updatedCells[globalIndex].diameterNm = static_cast<float>(diameterNm);
        }
    }

    // Для всех остальных клеток (из других файлов) применяем коэффициент
    if (currentCoeff > 0.0) {
        for (auto it = m_cellsByFile.begin(); it != m_cellsByFile.end(); ++it) {
            if (it.key() == m_currentFilePath) continue; // Текущий файл уже обработан

            QVector<int> cellIndices = it.value();
            for (int globalIndex : cellIndices) {
                double diameterNm = updatedCells[globalIndex].diameterPx * currentCoeff;
                updatedCells[globalIndex].diameter_um = diameterNm;
                updatedCells[globalIndex].diameterNm = static_cast<float>(diameterNm);
            }
        }
    }

    return updatedCells;
}

void VerificationWidget::onSaveCellsClicked()
{
    LOG_INFO("Save cells button clicked");

    // Получаем коэффициент из настроек
    double currentCoeff = SettingsManager::instance().getCoefficient();

    // Collect verified cells data for export
    QVector<QPair<Cell, double>> verifiedCells;

    // Get diameters from widgets for current file
    QVector<int> currentIndices = m_cellsByFile[m_currentFilePath];
    for (int i = 0; i < m_cellWidgets.size() && i < currentIndices.size(); ++i) {
        if (!m_cellWidgets[i]) continue;

        int globalIndex = currentIndices[i];
        double diameterNm = m_cellWidgets[i]->getDiameterNm();

        // Если поле пустое (0.0) и есть коэффициент, применяем его
        if (diameterNm == 0.0 && currentCoeff > 0.0) {
            diameterNm = m_cells[globalIndex].diameterPx * currentCoeff;
        }

        verifiedCells.append(qMakePair(m_cells[globalIndex], diameterNm));
    }

    // Add cells from other files (with coefficient if available)
    for (auto it = m_cellsByFile.begin(); it != m_cellsByFile.end(); ++it) {
        if (it.key() == m_currentFilePath) continue; // Skip current file, already added

        QVector<int> cellIndices = it.value();
        for (int globalIndex : cellIndices) {
            const Cell& cell = m_cells[globalIndex];
            double diameterNm = currentCoeff > 0 ? cell.diameterPx * currentCoeff : 0.0;
            verifiedCells.append(qMakePair(cell, diameterNm));
        }
    }

    if (verifiedCells.isEmpty()) {
        QMessageBox::information(this, "Информация", "Нет данных для сохранения.");
        return;
    }

    // Create results directory
    QString resultsDir = QDir::currentPath() + "/results";
    QDir().mkpath(resultsDir);

    // Generate CSV export
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd_hh-mm-ss");
    QString csvPath = resultsDir + QString("/cell_analysis_%1.csv").arg(timestamp);

    QFile csvFile(csvPath);
    if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&csvFile);
        stream << "filename,cell_number,center_x,center_y,diameter_pixels,diameter_um\n";

        QSet<QString> processedImages;
        int cellNumber = 1;

        for (const auto& cellPair : verifiedCells) {
            const Cell& cell = cellPair.first;
            double diameterNm = cellPair.second;

            QString imagePath = QString::fromStdString(cell.imagePath);
            QString filename = QFileInfo(imagePath).fileName();

            cv::Vec3f circle = cell.circle;
            int centerX = cvRound(circle[0]);
            int centerY = cvRound(circle[1]);

            stream << QString("%1,%2,%3,%4,%5,%6\n")
                .arg(filename)
                .arg(cellNumber++)
                .arg(centerX)
                .arg(centerY)
                .arg(cell.diameterPx)
                .arg(diameterNm, 0, 'f', 2);

            processedImages.insert(imagePath);
        }

        csvFile.close();
        LOG_INFO(QString("CSV exported to: %1").arg(csvPath));

        // Save debug images with highlighted cells
        for (const QString& imagePath : processedImages) {
            QVector<QPair<Cell, double>> imageCells;
            for (const auto& cellPair : verifiedCells) {
                if (QString::fromStdString(cellPair.first.imagePath) == imagePath) {
                    imageCells.append(cellPair);
                }
            }

            QString debugFileName = QFileInfo(imagePath).baseName() + "_highlighted.png";
            QString debugPath = resultsDir + "/" + debugFileName;
            saveDebugImage(imagePath, imageCells, debugPath);
        }

        // Log coefficient info
        if (currentCoeff > 0.0) {
            LOG_INFO(QString("Used coefficient: %1 μm/px").arg(currentCoeff, 0, 'f', 4));
        }

        QMessageBox::information(this, "Успех",
            QString("Результаты сохранены:\n- CSV: %1\n- Папка с результатами: %2")
            .arg(QFileInfo(csvPath).fileName())
            .arg(resultsDir));
    } else {
        QMessageBox::critical(this, "Ошибка", "Не удалось создать файл CSV.");
        LOG_ERROR(QString("Failed to create CSV file: %1").arg(csvPath));
    }
}

void VerificationWidget::saveDebugImage(const QString& originalImagePath,
                                       const QVector<QPair<Cell, double>>& cells,
                                       const QString& outputPath)
{
    LOG_INFO(QString("saveDebugImage: %1, cells=%2").arg(originalImagePath).arg(cells.size()));

    cv::Mat originalImage = loadImageSafely(originalImagePath);
    if (originalImage.empty()) {
        LOG_ERROR(QString("Failed to load image for debug: %1").arg(originalImagePath));
        return;
    }

    for (const auto& cellPair : cells) {
        const Cell& cell = cellPair.first;
        cv::Vec3f circle = cell.circle;

        int x = cvRound(circle[0]);
        int y = cvRound(circle[1]);
        int r = cvRound(circle[2]);

        if (x - r >= 0 && y - r >= 0 && x + r < originalImage.cols && y + r < originalImage.rows) {
            cv::Rect rect(x - r, y - r, 2 * r, 2 * r);
            cv::rectangle(originalImage, rect, cv::Scalar(0, 0, 255), 2);

            double diameterNm = cellPair.second;
            if (diameterNm > 0) {
                std::string text = std::to_string(static_cast<int>(diameterNm)) + " um";
                cv::putText(originalImage, text, cv::Point(x - r, y - r - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1);
            }
        }
    }

    bool saved = cv::imwrite(outputPath.toStdString(), originalImage);
    if (saved) {
        LOG_INFO(QString("Debug image saved: %1").arg(outputPath));
    } else {
        LOG_ERROR(QString("Failed to save debug image: %1").arg(outputPath));
    }
}

cv::Mat VerificationWidget::loadImageSafely(const QString& imagePath)
{
    // Check if path contains non-ASCII characters (Cyrillic, etc.)
    bool hasUnicode = false;
    for (QChar c : imagePath) {
        if (c.unicode() > 127) {
            hasUnicode = true;
            break;
        }
    }

    // For Unicode paths, use QImage directly to avoid OpenCV warnings
    if (hasUnicode) {
        QImage qImage;
        if (!qImage.load(imagePath)) {
            LOG_ERROR("Failed to load image: " + imagePath);
            return cv::Mat();
        }

        // Convert QImage to cv::Mat
        QImage rgbImage = qImage.convertToFormat(QImage::Format_RGB888);
        cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3,
                   (void*)rgbImage.constBits(), rgbImage.bytesPerLine());
        cv::Mat result;
        cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);

        LOG_DEBUG("Image loaded through QImage (Unicode path): " + imagePath);
        return result.clone();
    }

    // For ASCII paths, try OpenCV directly (faster)
    cv::Mat image = cv::imread(imagePath.toStdString());

    if (!image.empty()) {
        return image;
    }

    // Fallback to QImage if OpenCV failed
    QImage qImage;
    if (!qImage.load(imagePath)) {
        LOG_ERROR("Failed to load image: " + imagePath);
        return cv::Mat();
    }

    QImage rgbImage = qImage.convertToFormat(QImage::Format_RGB888);
    cv::Mat mat(rgbImage.height(), rgbImage.width(), CV_8UC3, (void*)rgbImage.constBits(), rgbImage.bytesPerLine());
    cv::Mat result;
    cv::cvtColor(mat, result, cv::COLOR_RGB2BGR);

    LOG_INFO("Image loaded through QImage (fallback): " + imagePath);
    return result.clone();
}

void VerificationWidget::loadNextThumbnailBatch()
{
    // Загружаем по 5 thumbnail за раз для плавности
    const int batchSize = 5;
    int loaded = 0;

    while (loaded < batchSize && m_thumbnailLoadIndex < m_cellWidgets.size()) {
        CellListItemWidget* widget = m_cellWidgets[m_thumbnailLoadIndex];
        if (widget) {
            widget->loadThumbnail();
        }
        m_thumbnailLoadIndex++;
        loaded++;
    }

    // Останавливаем таймер, когда все загружено
    if (m_thumbnailLoadIndex >= m_cellWidgets.size()) {
        if (m_thumbnailLoadTimer) {
            m_thumbnailLoadTimer->stop();
        }
        LOG_INFO(QString("All thumbnails loaded (%1 total)").arg(m_cellWidgets.size()));
    }
}

void VerificationWidget::onEditCoefficientClicked()
{
    // Toggle edit mode for coefficient field
    bool isReadOnly = m_coefficientEdit->isReadOnly();

    if (isReadOnly) {
        // Enable editing
        m_coefficientEdit->setReadOnly(false);
        m_coefficientEdit->setFocus();
        m_coefficientEdit->selectAll();
        m_editCoefficientButton->setText("💾");  // Save icon
        m_editCoefficientButton->setToolTip("Сохранить коэффициент");
        m_coefficientEdit->setStyleSheet("QLineEdit { background-color: #FFFDE7; border: 2px solid #FFC107; }");  // Yellow highlight

        LOG_INFO("Coefficient editing enabled");
    } else {
        // Finish editing (validate and save)
        onCoefficientEditingFinished();
    }
}

void VerificationWidget::onCoefficientEditingFinished()
{
    if (m_coefficientEdit->isReadOnly()) {
        return;  // Already in read-only mode
    }

    QString text = m_coefficientEdit->text().trimmed();

    // Validate input
    bool ok;
    double coefficient = text.toDouble(&ok);

    if (!ok || coefficient <= 0) {
        QMessageBox::warning(this, "Ошибка",
            "Неверное значение коэффициента. Введите положительное число.");
        m_coefficientEdit->setFocus();
        m_coefficientEdit->selectAll();
        return;
    }

    // Limit to 5 decimal places
    coefficient = std::round(coefficient * 100000.0) / 100000.0;

    // Update display
    m_coefficientEdit->setText(QString::number(coefficient, 'f', 5));
    m_coefficientEdit->setReadOnly(true);
    m_coefficientEdit->setStyleSheet("");  // Remove yellow highlight
    m_editCoefficientButton->setText("✏️");  // Pencil icon
    m_editCoefficientButton->setToolTip("Редактировать коэффициент");

    // Save to settings
    SettingsManager::instance().setCoefficient(coefficient);

    LOG_INFO(QString("Coefficient manually set to: %1 μm/px").arg(coefficient, 0, 'f', 5));

    // Recalculate all cell diameters with new coefficient
    recalculateDiameters();

    QMessageBox::information(this, "Успешно",
        QString("Коэффициент установлен: %1 мкм/px\nРазмеры клеток пересчитаны.").arg(coefficient, 0, 'f', 5));
}
