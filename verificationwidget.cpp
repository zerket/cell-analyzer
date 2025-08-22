// verificationwidget.cpp
// verificationwidget.cpp - FIXED VERSION
#include "verificationwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QListWidgetItem>
#include <QDir>
#include <QStandardPaths>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QImage>
#include <QResizeEvent>
#include <QTimer>
#include <QSet>
#include <QSettings>
#include <memory>
#include "utils.h"
#include "logger.h"
#include "settingsmanager.h"

VerificationWidget::VerificationWidget(const QVector<Cell>& cells, QWidget *parent)
    : QWidget(parent), m_cells(cells), listWidget(nullptr), m_resizeTimer(nullptr)
{
    LOG_INFO("VerificationWidget constructor called");
    LOG_INFO(QString("Received %1 cells").arg(cells.size()));
    
    try {
        setupUI();
        setupGridView();
        loadSavedCoefficient();
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Exception in VerificationWidget constructor: %1").arg(e.what()));
        QMessageBox::critical(this, "Error", QString("Failed to initialize: %1").arg(e.what()));
    }
}

VerificationWidget::~VerificationWidget()
{
    LOG_INFO("VerificationWidget destructor called");
    cleanupWidgets();
}

void VerificationWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    LOG_INFO("Main layout created");

    // View mode panel
    QHBoxLayout* viewModeLayout = new QHBoxLayout();
    viewModeLayout->addStretch();
    viewModeLayout->addWidget(new QLabel("Вид:"));
    
    gridViewButton = new QRadioButton("Сетка");
    listViewButton = new QRadioButton("Список");
    gridViewButton->setChecked(true);
    
    viewModeGroup = new QButtonGroup(this);
    viewModeGroup->addButton(gridViewButton, 0);
    viewModeGroup->addButton(listViewButton, 1);
    connect(viewModeGroup, &QButtonGroup::idClicked, 
            this, &VerificationWidget::onViewModeChanged);
    
    viewModeLayout->addWidget(gridViewButton);
    viewModeLayout->addWidget(listViewButton);
    viewModeLayout->addSpacing(20);
    mainLayout->addLayout(viewModeLayout);

    // ScrollArea for content
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    cellsContainer = new QWidget();
    scrollArea->setWidget(cellsContainer);
    mainLayout->addWidget(scrollArea);
    
    // Bottom panel
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    
    // Coefficient
    QLabel* coeffLabel = new QLabel("Коэффициент (мкм/пиксель):");
    bottomLayout->addWidget(coeffLabel);
    
    coefficientEdit = new QLineEdit();
    coefficientEdit->setReadOnly(true);
    coefficientEdit->setMaximumWidth(100);
    coefficientEdit->setPlaceholderText("Не определен");
    bottomLayout->addWidget(coefficientEdit);
    
    // Recalculate button next to coefficient
    recalcButton = new QPushButton("Пересчитать", this);
    recalcButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 5px 15px; }");
    connect(recalcButton, &QPushButton::clicked, this, &VerificationWidget::onRecalculateClicked);
    bottomLayout->addWidget(recalcButton);
    
    bottomLayout->addStretch();

    finishButton = new QPushButton("Завершить", this);
    finishButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 5px 15px; }");
    connect(finishButton, &QPushButton::clicked, this, &VerificationWidget::analysisCompleted);
    bottomLayout->addWidget(finishButton);

    saveButton = new QPushButton("Сохранить клетки", this);
    saveButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 5px 15px; }");
    connect(saveButton, &QPushButton::clicked, this, &VerificationWidget::onSaveCellsClicked);
    bottomLayout->addWidget(saveButton);

    clearDiametersButton = new QPushButton("Очистить", this);
    clearDiametersButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 5px 15px; }");
    connect(clearDiametersButton, &QPushButton::clicked, this, &VerificationWidget::onClearDiametersClicked);
    bottomLayout->addWidget(clearDiametersButton);
    
    // Добавляем разделитель
    bottomLayout->addStretch();
    
    statisticsButton = new QPushButton("📊 Статистика", this);
    statisticsButton->setStyleSheet("QPushButton { background-color: #9C27B0; color: white; border-radius: 10px; padding: 8px 16px; font-weight: bold; }");
    connect(statisticsButton, &QPushButton::clicked, this, &VerificationWidget::statisticsRequested);
    bottomLayout->addWidget(statisticsButton);

    mainLayout->addLayout(bottomLayout);
    setLayout(mainLayout);
    
    // Initialize resize timer for optimization
    m_resizeTimer = new QTimer(this);
    m_resizeTimer->setSingleShot(true);
    m_resizeTimer->setInterval(200); // 200ms delay
    connect(m_resizeTimer, &QTimer::timeout, this, &VerificationWidget::performDelayedResize);
    
    LOG_INFO("VerificationWidget UI setup completed");
}

void VerificationWidget::cleanupWidgets()
{
    LOG_INFO("Cleaning up widgets");
    
    if (listWidget) {
        listWidget->clear();
        listWidget->deleteLater();
        listWidget = nullptr;
    }
    
    // Clean up ALL child widgets in cellsContainer
    if (cellsContainer) {
        QList<QWidget*> children = cellsContainer->findChildren<QWidget*>();
        for (QWidget* child : children) {
            child->deleteLater();
        }
        
        // Clean up layout
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            while (QLayoutItem* item = layout->takeAt(0)) {
                delete item;
            }
            delete layout;
            cellsContainer->setLayout(nullptr);
        }
    }
    
    // Clear stored widget lists
    m_cellWidgets.clear();
    listViewDiameterEdits.clear();
    
    LOG_INFO("Widget cleanup completed");
}

void VerificationWidget::setupGridView()
{
    LOG_INFO("setupGridView() called");
    LOG_INFO(QString("Number of cells to display: %1").arg(m_cells.size()));
    
    cleanupWidgets();
    
    // Create new vertical layout to accommodate file labels
    QVBoxLayout* mainLayout = new QVBoxLayout(cellsContainer);
    mainLayout->setSpacing(20);
    
    // Group cells by file path
    QMap<QString, QVector<int>> cellsByFile;
    for (int i = 0; i < m_cells.size(); ++i) {
        QString imagePath = QString::fromStdString(m_cells[i].imagePath);
        cellsByFile[imagePath].append(i);
    }
    
    // Calculate columns dynamically
    int containerWidth = scrollArea->width() - 30;
    int cellWidgetWidth = 160;
    int spacing = 10;
    int maxCols = qMax(1, containerWidth / (cellWidgetWidth + spacing));
    
    LOG_INFO(QString("Grid layout: width=%1, columns=%2, files=%3").arg(containerWidth).arg(maxCols).arg(cellsByFile.size()));
    
    m_cellWidgets.clear();
    m_cellWidgets.reserve(m_cells.size());
    
    // Process each file group
    for (auto it = cellsByFile.begin(); it != cellsByFile.end(); ++it) {
        const QString& filePath = it.key();
        const QVector<int>& cellIndices = it.value();
        
        // Add file label
        QLabel* fileLabel = new QLabel(QFileInfo(filePath).fileName());
        QFont font = fileLabel->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 2);
        fileLabel->setFont(font);
        fileLabel->setStyleSheet("QLabel { color: #2196F3; padding: 10px 0px 5px 0px; }");
        mainLayout->addWidget(fileLabel);
        
        // Create grid for this file's cells
        QWidget* fileContainer = new QWidget();
        QGridLayout* gridLayout = new QGridLayout(fileContainer);
        gridLayout->setSpacing(10);
        
        int col = 0;
        int row = 0;
        
        for (int cellIndex : cellIndices) {
            try {
                auto cellWidget = std::make_unique<CellItemWidget>(m_cells[cellIndex]);
                
                // Автоматически заполняем диаметр если есть коэффициент
                double currentCoeff = SettingsManager::instance().getNmPerPixel();
                LOG_INFO(QString("Клетка %1: diameterPx=%2, diameterNm=%3, коэфф=%4")
                    .arg(cellIndex).arg(m_cells[cellIndex].diameterPx).arg(m_cells[cellIndex].diameterNm).arg(currentCoeff));
                if (currentCoeff > 0.0) {
                    double calculatedDiameter = m_cells[cellIndex].diameterPx * currentCoeff;
                    cellWidget->setDiameterNm(calculatedDiameter);
                    LOG_INFO(QString("Автоматически заполнен диаметр клетки %1: %2 нм (коэфф=%3)")
                        .arg(cellIndex).arg(calculatedDiameter, 0, 'f', 2).arg(currentCoeff, 0, 'f', 4));
                } else {
                    LOG_INFO(QString("Коэффициент не найден или равен 0 для клетки %1").arg(cellIndex));
                }
                
                connect(cellWidget.get(), &CellItemWidget::diameterNmChanged, 
                        this, &VerificationWidget::onDiameterNmChanged);
                connect(cellWidget.get(), &CellItemWidget::removeRequested, 
                        this, &VerificationWidget::onRemoveCellRequested);
                
                gridLayout->addWidget(cellWidget.get(), row, col);
                m_cellWidgets.append(cellWidget.release());
                
                col++;
                if (col >= maxCols) {
                    col = 0;
                    row++;
                }
            } catch (const std::exception& e) {
                LOG_ERROR(QString("Failed to create CellItemWidget %1: %2").arg(cellIndex).arg(e.what()));
            }
        }
        
        mainLayout->addWidget(fileContainer);
    }
    
    // Add stretch to push everything to the top
    mainLayout->addStretch();
    
    LOG_INFO(QString("setupGridView() completed. Created %1 widgets").arg(m_cellWidgets.size()));
}

void VerificationWidget::setupListView()
{
    LOG_INFO("setupListView() called");
    
    cleanupWidgets();
    
    // Create vertical layout for list
    QVBoxLayout* listLayout = new QVBoxLayout(cellsContainer);
    listLayout->setSpacing(20);
    
    // Group cells by file path
    QMap<QString, QVector<int>> cellsByFile;
    for (int i = 0; i < m_cells.size(); ++i) {
        QString imagePath = QString::fromStdString(m_cells[i].imagePath);
        cellsByFile[imagePath].append(i);
    }
    
    listViewDiameterEdits.clear();
    listViewDiameterEdits.reserve(m_cells.size());
    
    // Process each file group
    for (auto it = cellsByFile.begin(); it != cellsByFile.end(); ++it) {
        const QString& filePath = it.key();
        const QVector<int>& cellIndices = it.value();
        
        // Add file label
        QLabel* fileLabel = new QLabel(QFileInfo(filePath).fileName());
        QFont font = fileLabel->font();
        font.setBold(true);
        font.setPointSize(font.pointSize() + 2);
        fileLabel->setFont(font);
        fileLabel->setStyleSheet("QLabel { color: #2196F3; padding: 10px 0px 5px 0px; }");
        listLayout->addWidget(fileLabel);
        
        for (int cellIndex : cellIndices) {
            const Cell& cell = m_cells[cellIndex];
            
            QWidget* itemContainer = new QWidget();
            QHBoxLayout* itemLayout = new QHBoxLayout(itemContainer);
            itemLayout->setContentsMargins(10, 5, 10, 5);
            
            // Image on the left
            QLabel* imageLabel = new QLabel();
            QImage img = matToQImage(cell.image);
            if (!img.isNull()) {
                imageLabel->setPixmap(QPixmap::fromImage(img).scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
            imageLabel->setFixedSize(100, 100);
            itemLayout->addWidget(imageLabel);
            
            // Info on the right
            QWidget* infoWidget = new QWidget();
            QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);
            
            QLabel* diameterPxLabel = new QLabel(QString("Диаметр (px): %1").arg(cell.diameterPx));
            infoLayout->addWidget(diameterPxLabel);
            
            QHBoxLayout* nmLayout = new QHBoxLayout();
            nmLayout->addWidget(new QLabel("Диаметр (мкм):"));
            QLineEdit* diameterNmEdit = new QLineEdit();
            diameterNmEdit->setPlaceholderText("Введите значение");
            diameterNmEdit->setMaximumWidth(150);
            nmLayout->addWidget(diameterNmEdit);
            nmLayout->addStretch();
            infoLayout->addLayout(nmLayout);
            
            listViewDiameterEdits.append(diameterNmEdit);
            
            QPushButton* removeButton = new QPushButton("Удалить");
            removeButton->setMaximumWidth(100);
            removeButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 3px 10px; }");
            infoLayout->addWidget(removeButton);
            
            itemLayout->addWidget(infoWidget);
            itemLayout->addStretch();
            
            // Connect signals
            connect(diameterNmEdit, &QLineEdit::textChanged, this, &VerificationWidget::onDiameterNmChanged);
            
            int currentIndex = cellIndex; // Capture the current cellIndex for the lambda
            connect(removeButton, &QPushButton::clicked, [this, currentIndex]() {
                onRemoveCellAtIndex(currentIndex);
            });
            
            listLayout->addWidget(itemContainer);
        }
    }
    
    listLayout->addStretch();
    LOG_INFO("setupListView() completed");
}

void VerificationWidget::onRemoveCellAtIndex(int index)
{
    if (index >= 0 && index < m_cells.size()) {
        m_cells.removeAt(index);
        
        if (gridViewButton->isChecked()) {
            setupGridView();
        } else {
            setupListView();
        }
    }
}

void VerificationWidget::onRemoveCellRequested(CellItemWidget* item)
{
    if (!item) return;
    
    int index = m_cellWidgets.indexOf(item);
    if (index >= 0) {
        onRemoveCellAtIndex(index);
    }
}

void VerificationWidget::loadSavedCoefficient()
{
    double savedCoeff = SettingsManager::instance().getNmPerPixel();
    LOG_INFO(QString("loadSavedCoefficient: savedCoeff=%1").arg(savedCoeff));
    if (savedCoeff > 0) {
        coefficientEdit->setText(QString::number(savedCoeff, 'f', 4));
        
        // Автоматически заполняем диаметры всех клеток
        if (gridViewButton->isChecked()) {
            // Grid view mode
            LOG_INFO(QString("Grid view: %1 widgets, %2 cells").arg(m_cellWidgets.size()).arg(m_cells.size()));
            for (int i = 0; i < m_cellWidgets.size() && i < m_cells.size(); ++i) {
                if (m_cellWidgets[i]) {
                    double calculatedDiameter = m_cells[i].diameterPx * savedCoeff;
                    m_cellWidgets[i]->setDiameterNm(calculatedDiameter);
                    LOG_INFO(QString("loadSavedCoefficient: заполнен диаметр клетки %1: %2 нм").arg(i).arg(calculatedDiameter, 0, 'f', 2));
                }
            }
        } else {
            // List view mode
            LOG_INFO(QString("List view: %1 edits, %2 cells").arg(listViewDiameterEdits.size()).arg(m_cells.size()));
            for (int i = 0; i < listViewDiameterEdits.size() && i < m_cells.size(); ++i) {
                if (listViewDiameterEdits[i]) {
                    double calculatedDiameter = m_cells[i].diameterPx * savedCoeff;
                    listViewDiameterEdits[i]->setText(QString::number(calculatedDiameter, 'f', 2));
                }
            }
        }
        
        LOG_INFO(QString("Автоматически заполнены диаметры с коэффициентом %1 нм/px").arg(savedCoeff, 0, 'f', 4));
    } else {
        LOG_INFO("loadSavedCoefficient: коэффициент не найден или равен 0");
    }
}

void VerificationWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    
    // Use timer to prevent excessive rebuilding during resize
    if (gridViewButton && gridViewButton->isChecked() && m_resizeTimer) {
        m_resizeTimer->stop();
        m_resizeTimer->start();
    }
}

void VerificationWidget::performDelayedResize()
{
    if (gridViewButton && gridViewButton->isChecked()) {
        setupGridView();
    }
}

void VerificationWidget::onRecalculateClicked()
{
    recalculateDiameters();
}

void VerificationWidget::recalculateDiameters()
{
    QVector<double> scales;
    
    if (listViewButton->isChecked()) {
        // List view mode
        for (int i = 0; i < listViewDiameterEdits.size() && i < m_cells.size(); ++i) {
            QString nmText = listViewDiameterEdits[i]->text();
            if (!nmText.isEmpty()) {
                bool ok;
                double nm = nmText.toDouble(&ok);
                if (ok && m_cells[i].diameterPx > 0) {
                    scales.append(nm / m_cells[i].diameterPx);
                }
            }
        }
    } else {
        // Grid view mode
        for (CellItemWidget* widget : m_cellWidgets) {
            if (!widget) continue;
            QString nmText = widget->diameterNmText();
            if (!nmText.isEmpty()) {
                bool ok;
                double nm = nmText.toDouble(&ok);
                if (ok && widget->diameterPx() > 0) {
                    scales.append(nm / widget->diameterPx());
                }
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
    
    // Apply average scale to empty fields
    if (listViewButton->isChecked()) {
        for (int i = 0; i < listViewDiameterEdits.size() && i < m_cells.size(); ++i) {
            if (listViewDiameterEdits[i]->text().isEmpty()) {
                double nmValue = m_cells[i].diameterPx * avgScale;
                listViewDiameterEdits[i]->setText(QString::number(nmValue, 'f', 2));
            }
        }
    } else {
        for (CellItemWidget* widget : m_cellWidgets) {
            if (widget && widget->diameterNmText().isEmpty()) {
                double nmValue = widget->diameterPx() * avgScale;
                widget->setDiameterNm(nmValue);
            }
        }
    }
    
    // Update coefficient
    coefficientEdit->setText(QString::number(avgScale, 'f', 4));
    SettingsManager::instance().setNmPerPixel(avgScale);
    
    LOG_INFO(QString("Recalculated with coefficient: %1 μm/px").arg(avgScale));
}

void VerificationWidget::onDiameterNmChanged()
{
    updateRecalcButtonState();
}

void VerificationWidget::updateRecalcButtonState()
{
    // Enable recalc button if any diameter field has a value
    bool anyFilled = false;
    
    if (listViewButton->isChecked()) {
        for (QLineEdit* edit : listViewDiameterEdits) {
            if (edit && !edit->text().isEmpty()) {
                anyFilled = true;
                break;
            }
        }
    } else {
        for (CellItemWidget* widget : m_cellWidgets) {
            if (widget && !widget->diameterNmText().isEmpty()) {
                anyFilled = true;
                break;
            }
        }
    }
    
    recalcButton->setEnabled(anyFilled);
}

void VerificationWidget::onClearDiametersClicked()
{
    if (listViewButton->isChecked()) {
        for (QLineEdit* edit : listViewDiameterEdits) {
            if (edit) edit->clear();
        }
    } else {
        for (CellItemWidget* widget : m_cellWidgets) {
            if (widget) widget->setDiameterNm(0.0);
        }
    }
    
    coefficientEdit->clear();
    updateRecalcButtonState();
}

void VerificationWidget::onViewModeChanged(int mode)
{
    if (mode == 0) {
        setupGridView();
    } else {
        setupListView();
    }
}


void VerificationWidget::onSaveCellsClicked() {
    LOG_INFO("Save cells button clicked");
    
    // Collect verified cells data for export
    QVector<QPair<Cell, double>> verifiedCells;
    
    if (listViewButton->isChecked()) {
        // List view mode
        for (int i = 0; i < listViewDiameterEdits.size() && i < m_cells.size(); ++i) {
            QString nmText = listViewDiameterEdits[i]->text();
            double diameterNm = 0.0;
            if (!nmText.isEmpty()) {
                bool ok;
                diameterNm = nmText.toDouble(&ok);
                if (!ok) diameterNm = 0.0;
            }
            verifiedCells.append(qMakePair(m_cells[i], diameterNm));
        }
    } else {
        // Grid view mode
        for (int i = 0; i < m_cellWidgets.size() && i < m_cells.size(); ++i) {
            if (!m_cellWidgets[i]) continue;
            
            QString nmText = m_cellWidgets[i]->diameterNmText();
            double diameterNm = 0.0;
            if (!nmText.isEmpty()) {
                bool ok;
                diameterNm = nmText.toDouble(&ok);
                if (!ok) diameterNm = 0.0;
            }
            verifiedCells.append(qMakePair(m_cells[i], diameterNm));
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
        
        // Обновляем последний используемый пресет с текущим коэффициентом
        double currentCoeff = SettingsManager::instance().getNmPerPixel();
        if (currentCoeff > 0.0) {
            QSettings settings("CellAnalyzer", "HoughParams");
            QString lastPreset = settings.value("lastSelectedPreset", "").toString();
            
            if (!lastPreset.isEmpty() && lastPreset != "По умолчанию") {
                // Загружаем существующие пресеты
                QMap<QString, QPair<QString, double>> presetCoeffs; // name -> (params as json, coefficient)
                int size = settings.beginReadArray("presets");
                for (int i = 0; i < size; ++i) {
                    settings.setArrayIndex(i);
                    QString name = settings.value("name").toString();
                    double coeff = settings.value("coefficient", 0.0).toDouble();
                    if (name == lastPreset) {
                        // Обновляем коэффициент для текущего пресета
                        settings.setValue("coefficient", currentCoeff);
                        LOG_INFO(QString("Обновлен коэффициент %1 нм/px в пресете '%2'")
                            .arg(currentCoeff, 0, 'f', 4).arg(lastPreset));
                        break;
                    }
                }
                settings.endArray();
            }
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
                                       const QString& outputPath) {
    LOG_INFO(QString("saveDebugImage called: originalImagePath=%1, outputPath=%2, cells count=%3")
        .arg(originalImagePath).arg(outputPath).arg(cells.size()));
    
    // Загружаем оригинальное изображение
    cv::Mat originalImage = loadImageSafely(originalImagePath);
    if (originalImage.empty()) {
        LOG_ERROR(QString("Failed to load image for debug: %1").arg(originalImagePath));
        qWarning() << "Не удалось загрузить изображение для debug:" << originalImagePath;
        return;
    }
    
    LOG_INFO(QString("Original image loaded: %1x%2").arg(originalImage.cols).arg(originalImage.rows));
    
    // Рисуем красные квадраты вокруг найденных клеток
    int drawnCount = 0;
    for (const auto& cellPair : cells) {
        const Cell& cell = cellPair.first;
        cv::Vec3f circle = cell.circle;
        
        int x = cvRound(circle[0]);
        int y = cvRound(circle[1]);
        int r = cvRound(circle[2]);
        
        // Проверяем границы
        if (x - r >= 0 && y - r >= 0 && 
            x + r < originalImage.cols && y + r < originalImage.rows) {
            // Рисуем красный прямоугольник
            cv::Rect rect(x - r, y - r, 2 * r, 2 * r);
            cv::rectangle(originalImage, rect, cv::Scalar(0, 0, 255), 2);
            
            // Добавляем подпись с размером
            double diameterNm = cellPair.second;
            if (diameterNm > 0) {
                std::string text = std::to_string(static_cast<int>(diameterNm)) + " μm";
                cv::putText(originalImage, text, 
                           cv::Point(x - r, y - r - 5),
                           cv::FONT_HERSHEY_SIMPLEX, 0.5, 
                           cv::Scalar(0, 0, 255), 1);
            }
            drawnCount++;
        }
    }
    
    LOG_INFO(QString("Drew %1 rectangles on debug image").arg(drawnCount));
    
    // Сохраняем debug изображение
    bool saved = cv::imwrite(outputPath.toStdString(), originalImage);
    if (saved) {
        LOG_INFO(QString("Debug image saved successfully to: %1").arg(outputPath));
    } else {
        LOG_ERROR(QString("Failed to save debug image to: %1").arg(outputPath));
    }
}

cv::Mat VerificationWidget::loadImageSafely(const QString& imagePath) {
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

