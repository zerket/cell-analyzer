// verificationwidget.cpp
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
#include "utils.h"
#include "logger.h"

VerificationWidget::VerificationWidget(const QVector<Cell>& cells, QWidget *parent)
    : QWidget(parent), m_cells(cells)
{
    LOG_INFO("VerificationWidget constructor called");
    LOG_INFO(QString("Received %1 cells").arg(cells.size()));
    
    try {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        LOG_INFO("Main layout created");

    // Панель переключения вида
    QHBoxLayout* viewModeLayout = new QHBoxLayout();
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
    viewModeLayout->addStretch();
    mainLayout->addLayout(viewModeLayout);

    // ScrollArea для контента
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    cellsContainer = new QWidget();
    scrollArea->setWidget(cellsContainer);
    mainLayout->addWidget(scrollArea);
    
    // Инициализируем как сетку по умолчанию
    setupGridView();

    QHBoxLayout* buttonsLayout = new QHBoxLayout();

    recalcButton = new QPushButton("Пересчитать", this);
    connect(recalcButton, &QPushButton::clicked, this, &VerificationWidget::onRecalculateClicked);
    buttonsLayout->addWidget(recalcButton);

    finishButton = new QPushButton("Завершить", this);
    connect(finishButton, &QPushButton::clicked, this, &VerificationWidget::analysisCompleted);
    buttonsLayout->addWidget(finishButton);

    saveButton = new QPushButton("Сохранить клетки", this);
    connect(saveButton, &QPushButton::clicked, this, &VerificationWidget::onSaveCellsClicked);
    buttonsLayout->addWidget(saveButton);

    clearDiametersButton = new QPushButton("Очистить", this);
    connect(clearDiametersButton, &QPushButton::clicked, this, &VerificationWidget::onClearDiametersClicked);
    buttonsLayout->addWidget(clearDiametersButton);

    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
    
    LOG_INFO("VerificationWidget layout completed");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Exception in VerificationWidget constructor: %1").arg(e.what()));
        throw;
    } catch (...) {
        LOG_ERROR("Unknown exception in VerificationWidget constructor");
        throw;
    }
    
    LOG_INFO("VerificationWidget constructor completed successfully");
}

void VerificationWidget::onDiameterNmChanged() {
    updateRecalcButtonState();
}

void VerificationWidget::updateRecalcButtonState() {
    bool anyFilled = false;
    
    if (listWidget) {
        for (int i = 0; i < listWidget->count(); ++i) {
            CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
            if (w && !w->diameterNmText().isEmpty()) {
                anyFilled = true;
                break;
            }
        }
    } else {
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                CellItemWidget* w = qobject_cast<CellItemWidget*>(layout->itemAt(i)->widget());
                if (w && !w->diameterNmText().isEmpty()) {
                    anyFilled = true;
                    break;
                }
            }
        }
    }
    //recalcButton->setEnabled(anyFilled);  // <- эта строка закомментирована
}

void VerificationWidget::onRecalculateClicked() {
    recalculateDiameters();
}

void VerificationWidget::recalculateDiameters() {
    QVector<double> scales;
    QVector<CellItemWidget*> widgets;
    
    // Собираем все виджеты
    if (listWidget) {
        for (int i = 0; i < listWidget->count(); ++i) {
            CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
            if (w) widgets.append(w);
        }
    } else {
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                CellItemWidget* w = qobject_cast<CellItemWidget*>(layout->itemAt(i)->widget());
                if (w) widgets.append(w);
            }
        }
    }

    // Собираем масштабы
    for (CellItemWidget* w : widgets) {
        QString nmText = w->diameterNmText();
        if (!nmText.isEmpty()) {
            double nm = nmText.toDouble();
            int px = w->diameterPx();
            if (px > 0) {
                scales.append(nm / px);
            }
        }
    }

    if (scales.isEmpty()) return;

    double avgScale = 0;
    for (double s : scales) avgScale += s;
    avgScale /= scales.size();

    // Применяем средний масштаб
    for (CellItemWidget* w : widgets) {
        if (w->diameterNmText().isEmpty()) {
            double nmValue = w->diameterPx() * avgScale;
            w->setDiameterNm(nmValue);
        }
    }
}

void VerificationWidget::onRemoveCellRequested(CellItemWidget* item) {
    // Найти индекс клетки
    int index = -1;
    if (listWidget) {
        for (int i = 0; i < listWidget->count(); ++i) {
            QListWidgetItem* listItem = listWidget->item(i);
            QWidget* widget = listWidget->itemWidget(listItem);
            if (widget == item) {
                index = i;
                break;
            }
        }
    } else {
        // Для режима сетки/списка находим индекс по виджету
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                QWidget* widget = layout->itemAt(i)->widget();
                if (widget == item) {
                    index = i;
                    break;
                }
            }
        }
    }
    
    if (index >= 0 && index < m_cells.size()) {
        m_cells.removeAt(index);
        // Перестраиваем текущий вид
        if (gridViewButton->isChecked()) {
            setupGridView();
        } else {
            setupListView();
        }
    }
}

void VerificationWidget::onSaveCellsClicked() {
    // Группируем клетки по изображениям
    QMap<QString, QVector<QPair<Cell, double>>> cellsByImage;
    
    // Собираем все виджеты и их данные
    QVector<CellItemWidget*> widgets;
    if (listWidget) {
        for (int i = 0; i < listWidget->count(); ++i) {
            CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
            if (w && i < m_cells.size()) {
                widgets.append(w);
                QString nmText = w->diameterNmText();
                double diameterNm = nmText.isEmpty() ? 0.0 : nmText.toDouble();
                cellsByImage[QString::fromStdString(m_cells[i].imagePath)].append(qMakePair(m_cells[i], diameterNm));
            }
        }
    } else {
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                CellItemWidget* w = qobject_cast<CellItemWidget*>(layout->itemAt(i)->widget());
                if (w && i < m_cells.size()) {
                    widgets.append(w);
                    QString nmText = w->diameterNmText();
                    double diameterNm = nmText.isEmpty() ? 0.0 : nmText.toDouble();
                    cellsByImage[QString::fromStdString(m_cells[i].imagePath)].append(qMakePair(m_cells[i], diameterNm));
                }
            }
        }
    }
    
    // Создаем папку results с датой
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString resultsPath = QDir::currentPath() + "/results/" + dateStr;
    QDir resultsDir(resultsPath);
    if (!resultsDir.exists()) {
        resultsDir.mkpath(".");
    }
    
    LOG_INFO(QString("Saving results to: %1").arg(resultsDir.absolutePath()));
    
    // Обрабатываем каждое изображение
    for (auto it = cellsByImage.begin(); it != cellsByImage.end(); ++it) {
        QString imagePath = it.key();
        QVector<QPair<Cell, double>> cells = it.value();
        
        // Получаем имя файла без расширения
        QFileInfo fileInfo(imagePath);
        QString baseName = fileInfo.baseName();
        
        // Создаем папку для этого изображения
        QDir imageDir(resultsDir.filePath(baseName));
        if (!imageDir.exists()) {
            imageDir.mkpath(".");
        }
        
        LOG_INFO(QString("Processing image: %1, cells count: %2").arg(baseName).arg(cells.size()));
        
        // Сохраняем debug изображение с квадратами
        saveDebugImage(imagePath, cells, imageDir.filePath("debug.png"));
        
        // Открываем CSV файл для записи
        QFile csvFile(imageDir.filePath("results.csv"));
        if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&csvFile);
            stream << "filename,diameter_nm\n";
            
            // Сохраняем отдельные изображения клеток
            int cellIndex = 1;
            for (const auto& cellPair : cells) {
                const Cell& cell = cellPair.first;
                double diameterNm = cellPair.second;
                
                // Формируем имя файла
                QString cellFileName = QString("cell_%1_%2nm.png")
                    .arg(cellIndex, 3, 10, QChar('0'))
                    .arg(static_cast<int>(diameterNm));
                
                // Сохраняем изображение клетки
                QImage img = matToQImage(cell.image);
                QString cellFilePath = imageDir.filePath(cellFileName);
                if (img.save(cellFilePath, "PNG")) {
                    // Записываем в CSV
                    stream << cellFileName << "," << QString::number(diameterNm, 'f', 2) << "\n";
                } else {
                    qWarning() << "Не удалось сохранить файл:" << cellFilePath;
                }
                
                cellIndex++;
            }
            
            csvFile.close();
        }
    }
    
    QMessageBox::information(this, "Сохранение завершено", 
        QString("Результаты сохранены в папку: %1").arg(resultsDir.absolutePath()));
}

void VerificationWidget::onClearDiametersClicked() {
    QVector<CellItemWidget*> widgets;
    if (listWidget) {
        for (int i = 0; i < listWidget->count(); ++i) {
            CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
            if (w) widgets.append(w);
        }
    } else {
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                CellItemWidget* w = qobject_cast<CellItemWidget*>(layout->itemAt(i)->widget());
                if (w) widgets.append(w);
            }
        }
    }
    
    for (CellItemWidget* w : widgets) {
        w->setDiameterNm(.0);
    }
    updateRecalcButtonState();
}

void VerificationWidget::onViewModeChanged(int mode) {
    if (mode == 0) {
        setupGridView();
    } else {
        setupListView();
    }
}

void VerificationWidget::setupGridView() {
    LOG_INFO("setupGridView() called");
    LOG_INFO(QString("Number of cells to display: %1").arg(m_cells.size()));
    
    // Удаляем старый виджет списка если есть
    if (listWidget) {
        listWidget->deleteLater();
        listWidget = nullptr;
    }
    
    // Очищаем контейнер
    QLayout* oldLayout = cellsContainer->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        delete oldLayout;
    }
    
    // Создаем grid layout
    QGridLayout* gridLayout = new QGridLayout(cellsContainer);
    gridLayout->setSpacing(10);
    
    int col = 0;
    int row = 0;
    int maxCols = 4; // Можно сделать адаптивным
    
    LOG_INFO("Starting to create cell widgets");
    int cellCount = 0;
    
    for (const Cell& cell : m_cells) {
        LOG_DEBUG(QString("Creating CellItemWidget for cell %1").arg(cellCount));
        
        CellItemWidget* cellWidget = nullptr;
        try {
            cellWidget = new CellItemWidget(cell);
            LOG_DEBUG(QString("CellItemWidget %1 created successfully").arg(cellCount));
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Failed to create CellItemWidget %1: %2").arg(cellCount).arg(e.what()));
            continue;
        }
        
        connect(cellWidget, &CellItemWidget::diameterNmChanged, this, &VerificationWidget::onDiameterNmChanged);
        connect(cellWidget, &CellItemWidget::removeRequested, this, &VerificationWidget::onRemoveCellRequested);
        
        gridLayout->addWidget(cellWidget, row, col);
        LOG_DEBUG(QString("Added cell %1 to grid at position (%2, %3)").arg(cellCount).arg(row).arg(col));
        
        col++;
        if (col >= maxCols) {
            col = 0;
            row++;
        }
        cellCount++;
    }
    
    LOG_INFO(QString("setupGridView() completed. Created %1 cell widgets").arg(cellCount));
}

void VerificationWidget::setupListView() {
    // Очищаем контейнер
    QLayout* oldLayout = cellsContainer->layout();
    if (oldLayout) {
        QLayoutItem* item;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        delete oldLayout;
    }
    
    // Создаем вертикальный layout для списка
    QVBoxLayout* listLayout = new QVBoxLayout(cellsContainer);
    listLayout->setSpacing(10);
    
    for (const Cell& cell : m_cells) {
        // Создаем горизонтальный контейнер для каждой клетки
        QWidget* itemContainer = new QWidget();
        QHBoxLayout* itemLayout = new QHBoxLayout(itemContainer);
        itemLayout->setContentsMargins(10, 5, 10, 5);
        
        // Изображение слева
        QLabel* imageLabel = new QLabel();
        QImage img = matToQImage(cell.image);
        imageLabel->setPixmap(QPixmap::fromImage(img).scaled(100, 100, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imageLabel->setFixedSize(100, 100);
        itemLayout->addWidget(imageLabel);
        
        // Информация справа
        QWidget* infoWidget = new QWidget();
        QVBoxLayout* infoLayout = new QVBoxLayout(infoWidget);
        
        QLabel* diameterPxLabel = new QLabel(QString("Диаметр (px): %1").arg(cell.diameterPx));
        infoLayout->addWidget(diameterPxLabel);
        
        QHBoxLayout* nmLayout = new QHBoxLayout();
        nmLayout->addWidget(new QLabel("Диаметр (нм):"));
        QLineEdit* diameterNmEdit = new QLineEdit();
        diameterNmEdit->setPlaceholderText("Введите значение");
        diameterNmEdit->setMaximumWidth(150);
        nmLayout->addWidget(diameterNmEdit);
        nmLayout->addStretch();
        infoLayout->addLayout(nmLayout);
        
        QPushButton* removeButton = new QPushButton("Удалить");
        removeButton->setMaximumWidth(100);
        infoLayout->addWidget(removeButton);
        
        itemLayout->addWidget(infoWidget);
        itemLayout->addStretch();
        
        // Создаем CellItemWidget для обработки (нужен для совместимости)
        CellItemWidget* cellWidget = new CellItemWidget(cell);
        cellWidget->hide(); // Скрываем, но используем для логики
        
        // Синхронизируем данные
        connect(diameterNmEdit, &QLineEdit::textChanged, [cellWidget](const QString& text) {
            cellWidget->setDiameterNm(text.toDouble());
        });
        connect(diameterNmEdit, &QLineEdit::textChanged, this, &VerificationWidget::onDiameterNmChanged);
        connect(removeButton, &QPushButton::clicked, [this, cellWidget]() {
            emit cellWidget->removeRequested(cellWidget);
        });
        connect(cellWidget, &CellItemWidget::removeRequested, this, &VerificationWidget::onRemoveCellRequested);
        
        // Сохраняем ссылку на виджет для работы с данными
        itemContainer->setProperty("cellWidget", QVariant::fromValue(cellWidget));
        
        listLayout->addWidget(itemContainer);
    }
    
    listLayout->addStretch();
}

void VerificationWidget::saveDebugImage(const QString& originalImagePath, 
                                       const QVector<QPair<Cell, double>>& cells,
                                       const QString& outputPath) {
    LOG_INFO(QString("saveDebugImage called: originalImagePath=%1, outputPath=%2, cells count=%3")
        .arg(originalImagePath).arg(outputPath).arg(cells.size()));
    
    // Загружаем оригинальное изображение
    cv::Mat originalImage = cv::imread(originalImagePath.toStdString());
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
                std::string text = std::to_string(static_cast<int>(diameterNm)) + " nm";
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
