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
#include <QResizeEvent>
#include <QTimer>
#include <QSet>
#include "utils.h"
#include "logger.h"
#include "settingsmanager.h"

VerificationWidget::VerificationWidget(const QVector<Cell>& cells, QWidget *parent)
    : QWidget(parent), m_cells(cells), listWidget(nullptr)
{
    LOG_INFO("VerificationWidget constructor called");
    LOG_INFO(QString("Received %1 cells").arg(cells.size()));
    
    try {
        QVBoxLayout* mainLayout = new QVBoxLayout(this);
        LOG_INFO("Main layout created");

    // Панель переключения вида
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

    // ScrollArea для контента
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    cellsContainer = new QWidget();
    scrollArea->setWidget(cellsContainer);
    mainLayout->addWidget(scrollArea);
    
    LOG_INFO("ScrollArea and cellsContainer created successfully");
    LOG_INFO(QString("cellsContainer pointer: %1").arg(reinterpret_cast<quintptr>(cellsContainer), 0, 16));
    
    // Инициализируем как сетку по умолчанию
    LOG_INFO("About to call setupGridView()");
    setupGridView();

    // Нижняя панель с кнопками и коэффициентом
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    
    // Коэффициент слева
    QLabel* coeffLabel = new QLabel("Коэффициент (нм/пиксель):");
    bottomLayout->addWidget(coeffLabel);
    
    coefficientEdit = new QLineEdit();
    coefficientEdit->setReadOnly(true);
    coefficientEdit->setMaximumWidth(100);
    coefficientEdit->setPlaceholderText("Не определен");
    
    // Загружаем сохраненный коэффициент
    double savedCoeff = SettingsManager::instance().getNmPerPixel();
    if (savedCoeff > 0) {
        coefficientEdit->setText(QString::number(savedCoeff, 'f', 4));
    }
    
    bottomLayout->addWidget(coefficientEdit);
    
    bottomLayout->addStretch();
    
    // Кнопки справа
    recalcButton = new QPushButton("Пересчитать", this);
    recalcButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 5px 15px; }");
    connect(recalcButton, &QPushButton::clicked, this, &VerificationWidget::onRecalculateClicked);
    bottomLayout->addWidget(recalcButton);

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

    mainLayout->addLayout(bottomLayout);
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
    
    // Проверяем, какой вид активен
    if (listViewButton->isChecked()) {
        // Режим списка - работаем с listViewDiameterEdits
        for (int i = 0; i < listViewDiameterEdits.size() && i < m_cells.size(); ++i) {
            QString nmText = listViewDiameterEdits[i]->text();
            if (!nmText.isEmpty()) {
                double nm = nmText.toDouble();
                int px = m_cells[i].diameterPx;
                if (px > 0) {
                    scales.append(nm / px);
                }
            }
        }
        
        if (scales.isEmpty()) return;
        
        double avgScale = 0;
        for (double s : scales) avgScale += s;
        avgScale /= scales.size();
        
        // Применяем средний масштаб к пустым полям
        for (int i = 0; i < listViewDiameterEdits.size() && i < m_cells.size(); ++i) {
            if (listViewDiameterEdits[i]->text().isEmpty()) {
                double nmValue = m_cells[i].diameterPx * avgScale;
                listViewDiameterEdits[i]->setText(QString::number(nmValue, 'f', 2));
            }
        }
        
        // Обновляем коэффициент
        coefficientEdit->setText(QString::number(avgScale, 'f', 4));
        // Сохраняем коэффициент в настройки
        SettingsManager::instance().setNmPerPixel(avgScale);
        
    } else {
        // Режим сетки - работаем с CellItemWidget
        QVector<CellItemWidget*> widgets;
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                CellItemWidget* w = qobject_cast<CellItemWidget*>(layout->itemAt(i)->widget());
                if (w) widgets.append(w);
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
        
        // Обновляем коэффициент
        coefficientEdit->setText(QString::number(avgScale, 'f', 4));
        // Сохраняем коэффициент в настройки
        SettingsManager::instance().setNmPerPixel(avgScale);
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
    
    // Проверяем количество изображений
    QSet<QString> uniqueImages;
    for (const Cell& cell : m_cells) {
        uniqueImages.insert(QString::fromStdString(cell.imagePath));
    }
    
    bool saveSeparately = true; // По умолчанию сохраняем отдельно
    
    // Если больше одного изображения, спрашиваем пользователя
    if (uniqueImages.size() > 1) {
        QMessageBox msgBox;
        msgBox.setWindowTitle("Выбор способа сохранения");
        msgBox.setText(QString("Обнаружено %1 изображений. Как сохранить результаты?").arg(uniqueImages.size()));
        msgBox.setInformativeText("Выберите способ сохранения результатов:");
        
        QPushButton* separateBtn = msgBox.addButton("В отдельные папки", QMessageBox::AcceptRole);
        QPushButton* singleBtn = msgBox.addButton("В одну папку", QMessageBox::AcceptRole);
        msgBox.addButton(QMessageBox::Cancel);
        
        msgBox.exec();
        
        if (msgBox.clickedButton() == separateBtn) {
            saveSeparately = true;
        } else if (msgBox.clickedButton() == singleBtn) {
            saveSeparately = false;
        } else {
            // Отмена
            return;
        }
    }
    
    // Собираем все виджеты и их данные
    LOG_INFO(QString("Collecting cell data. List view mode: %1").arg(listViewButton->isChecked()));
    LOG_INFO(QString("Total cells: %1").arg(m_cells.size()));
    
    if (listViewButton->isChecked()) {
        // В режиме списка собираем данные из listViewDiameterEdits
        LOG_INFO(QString("List view diameter edits count: %1").arg(listViewDiameterEdits.size()));
        for (int i = 0; i < listViewDiameterEdits.size() && i < m_cells.size(); ++i) {
            QString nmText = listViewDiameterEdits[i]->text();
            double diameterNm = nmText.isEmpty() ? 0.0 : nmText.toDouble();
            cellsByImage[QString::fromStdString(m_cells[i].imagePath)].append(qMakePair(m_cells[i], diameterNm));
            LOG_DEBUG(QString("Cell %1: diameter=%2nm, path=%3").arg(i).arg(diameterNm).arg(QString::fromStdString(m_cells[i].imagePath)));
        }
    } else {
        // В режиме сетки собираем данные из CellItemWidget
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            LOG_INFO(QString("Grid layout item count: %1").arg(layout->count()));
            for (int i = 0; i < layout->count() && i < m_cells.size(); ++i) {
                CellItemWidget* w = qobject_cast<CellItemWidget*>(layout->itemAt(i)->widget());
                if (w) {
                    QString nmText = w->diameterNmText();
                    double diameterNm = nmText.isEmpty() ? 0.0 : nmText.toDouble();
                    cellsByImage[QString::fromStdString(m_cells[i].imagePath)].append(qMakePair(m_cells[i], diameterNm));
                    LOG_DEBUG(QString("Cell %1: diameter=%2nm, path=%3").arg(i).arg(diameterNm).arg(QString::fromStdString(m_cells[i].imagePath)));
                }
            }
        }
    }
    
    LOG_INFO(QString("Total images with cells: %1").arg(cellsByImage.size()));
    for (auto it = cellsByImage.begin(); it != cellsByImage.end(); ++it) {
        LOG_INFO(QString("Image %1 has %2 cells").arg(it.key()).arg(it.value().size()));
    }
    
    // Создаем папку results с датой
    QString dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd_HH-mm-ss");
    QString resultsPath = QDir::currentPath() + "/results/" + dateStr;
    QDir resultsDir(resultsPath);
    if (!resultsDir.exists()) {
        resultsDir.mkpath(".");
    }
    
    LOG_INFO(QString("Saving results to: %1").arg(resultsDir.absolutePath()));
    
    if (saveSeparately) {
        // Сохраняем в отдельные папки (как было раньше)
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
    } else {
        // Сохраняем все в одну папку
        QFile csvFile(resultsDir.filePath("all_results.csv"));
        if (csvFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream stream(&csvFile);
            stream << "source_image,cell_filename,diameter_nm\n";
            
            int globalCellIndex = 1;
            
            for (auto it = cellsByImage.begin(); it != cellsByImage.end(); ++it) {
                QString imagePath = it.key();
                QVector<QPair<Cell, double>> cells = it.value();
                
                QFileInfo fileInfo(imagePath);
                QString baseName = fileInfo.baseName();
                
                // Сохраняем debug изображение
                saveDebugImage(imagePath, cells, resultsDir.filePath(QString("debug_%1.png").arg(baseName)));
                
                // Сохраняем клетки
                for (const auto& cellPair : cells) {
                    const Cell& cell = cellPair.first;
                    double diameterNm = cellPair.second;
                    
                    // Формируем имя файла с префиксом изображения
                    QString cellFileName = QString("%1_cell_%2_%3nm.png")
                        .arg(baseName)
                        .arg(globalCellIndex, 4, 10, QChar('0'))
                        .arg(static_cast<int>(diameterNm));
                    
                    // Сохраняем изображение клетки
                    QImage img = matToQImage(cell.image);
                    QString cellFilePath = resultsDir.filePath(cellFileName);
                    if (img.save(cellFilePath, "PNG")) {
                        // Записываем в CSV
                        stream << baseName << "," << cellFileName << "," << QString::number(diameterNm, 'f', 2) << "\n";
                    } else {
                        qWarning() << "Не удалось сохранить файл:" << cellFilePath;
                    }
                    
                    globalCellIndex++;
                }
            }
            
            csvFile.close();
        }
    }
    
    QMessageBox::information(this, "Сохранение завершено", 
        QString("Результаты сохранены в папку: %1").arg(resultsDir.absolutePath()));
}

void VerificationWidget::onClearDiametersClicked() {
    if (listViewButton->isChecked()) {
        // Очищаем поля в режиме списка
        for (QLineEdit* edit : listViewDiameterEdits) {
            edit->clear();
        }
    } else {
        // Очищаем поля в режиме сетки
        QVector<CellItemWidget*> widgets;
        QLayout* layout = cellsContainer->layout();
        if (layout) {
            for (int i = 0; i < layout->count(); ++i) {
                CellItemWidget* w = qobject_cast<CellItemWidget*>(layout->itemAt(i)->widget());
                if (w) widgets.append(w);
            }
        }
        
        for (CellItemWidget* w : widgets) {
            w->setDiameterNm(.0);
        }
    }
    
    // Очищаем коэффициент
    coefficientEdit->clear();
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
    
    LOG_INFO("Step 1: Checking and deleting old list widget");
    // Удаляем старый виджет списка если есть
    if (listWidget) {
        LOG_INFO("List widget exists, deleting it");
        listWidget->deleteLater();
        listWidget = nullptr;
    }
    
    LOG_INFO("Step 2: Getting old layout from cellsContainer");
    // Очищаем контейнер
    QLayout* oldLayout = cellsContainer->layout();
    if (oldLayout) {
        LOG_INFO("Old layout exists, clearing it");
        QLayoutItem* item;
        int itemCount = 0;
        while ((item = oldLayout->takeAt(0)) != nullptr) {
            LOG_INFO(QString("Removing item %1").arg(itemCount++));
            if (item->widget()) {
                item->widget()->deleteLater();
            }
            delete item;
        }
        LOG_INFO("Deleting old layout");
        delete oldLayout;
    }
    
    LOG_INFO("Step 3: Creating new grid layout");
    // Создаем grid layout
    QGridLayout* gridLayout = new QGridLayout(cellsContainer);
    gridLayout->setSpacing(10);
    LOG_INFO("Grid layout created and set to cellsContainer");
    
    // Вычисляем количество колонок динамически
    int containerWidth = scrollArea->width() - 30; // Учитываем полосу прокрутки
    int cellWidgetWidth = 160; // Ширина одного виджета клетки
    int spacing = 10;
    int maxCols = qMax(1, containerWidth / (cellWidgetWidth + spacing));
    LOG_INFO(QString("Dynamic grid: container width=%1, max columns=%2").arg(containerWidth).arg(maxCols));
    
    int col = 0;
    int row = 0;
    
    LOG_INFO("Starting to create cell widgets");
    LOG_INFO(QString("First, let's check the cells vector size: %1").arg(m_cells.size()));
    int cellCount = 0;
    
    for (const Cell& cell : m_cells) {
        LOG_INFO(QString("Creating CellItemWidget for cell %1 of %2").arg(cellCount).arg(m_cells.size()));
        LOG_INFO(QString("Cell %1 info: image size=%2x%3, diameterPx=%4")
            .arg(cellCount)
            .arg(cell.image.cols)
            .arg(cell.image.rows)
            .arg(cell.diameterPx));
        
        CellItemWidget* cellWidget = nullptr;
        try {
            cellWidget = new CellItemWidget(cell);
            LOG_INFO(QString("CellItemWidget %1 created successfully").arg(cellCount));
        } catch (const std::exception& e) {
            LOG_ERROR(QString("Failed to create CellItemWidget %1: %2").arg(cellCount).arg(e.what()));
            continue;
        } catch (...) {
            LOG_ERROR(QString("Unknown exception creating CellItemWidget %1").arg(cellCount));
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
    
    // Очищаем список редакторов диаметров
    listViewDiameterEdits.clear();
    
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
        
        // Сохраняем ссылку на редактор диаметра
        listViewDiameterEdits.append(diameterNmEdit);
        
        QPushButton* removeButton = new QPushButton("Удалить");
        removeButton->setMaximumWidth(100);
        removeButton->setStyleSheet("QPushButton { border: 1px solid #ccc; border-radius: 5px; padding: 3px 10px; }");
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
        itemContainer->setProperty("diameterPx", cell.diameterPx);
        
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

void VerificationWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    
    // Перестраиваем grid view только если он активен
    if (gridViewButton && gridViewButton->isChecked()) {
        // Используем таймер для предотвращения слишком частых обновлений
        static QTimer* resizeTimer = nullptr;
        if (!resizeTimer) {
            resizeTimer = new QTimer(this);
            resizeTimer->setSingleShot(true);
            connect(resizeTimer, &QTimer::timeout, [this]() {
                setupGridView();
            });
        }
        resizeTimer->stop();
        resizeTimer->start(200); // Задержка 200мс
    }
}
