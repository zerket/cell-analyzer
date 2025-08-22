#include "statisticswidget.h"
#include "logger.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QHeaderView>
#include <QSplitter>
#include <QGroupBox>
#include <QTextStream>
#include <QStringConverter>
#include <QApplication>

StatisticsWidget::StatisticsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

StatisticsWidget::~StatisticsWidget() {
}

void StatisticsWidget::setupUI() {
    setWindowTitle("Статистический анализ");
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // Заголовок
    QLabel* titleLabel = new QLabel("Статистический анализ результатов");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; margin: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // Создаем табы
    tabWidget = new QTabWidget();
    
    // Вкладка "Обзор"
    overviewTab = new QWidget();
    QVBoxLayout* overviewLayout = new QVBoxLayout(overviewTab);
    
    summaryText = new QTextEdit();
    summaryText->setMaximumHeight(150);
    summaryText->setReadOnly(true);
    overviewLayout->addWidget(new QLabel("Резюме анализа:"));
    overviewLayout->addWidget(summaryText);
    
    overviewTable = new QTableWidget();
    overviewLayout->addWidget(new QLabel("Основные статистики:"));
    overviewLayout->addWidget(overviewTable);
    
    tabWidget->addTab(overviewTab, "Обзор");
    
    // Вкладка "Детали"
    detailsTab = new QWidget();
    QVBoxLayout* detailsLayout = new QVBoxLayout(detailsTab);
    
    QSplitter* detailsSplitter = new QSplitter(Qt::Vertical);
    
    detailsTable = new QTableWidget();
    detailsSplitter->addWidget(detailsTable);
    
    imageGroupsTable = new QTableWidget();
    detailsSplitter->addWidget(imageGroupsTable);
    
    detailsLayout->addWidget(new QLabel("Подробная статистика:"));
    detailsLayout->addWidget(detailsSplitter);
    
    tabWidget->addTab(detailsTab, "Детали");
    
    // Вкладка "Распределения"
    distributionTab = new QWidget();
    QVBoxLayout* distributionLayout = new QVBoxLayout(distributionTab);
    
    distributionText = new QTextEdit();
    distributionText->setReadOnly(true);
    distributionLayout->addWidget(new QLabel("Анализ распределений:"));
    distributionLayout->addWidget(distributionText);
    
    tabWidget->addTab(distributionTab, "Распределения");
    
    // Вкладка "Корреляции"
    correlationTab = new QWidget();
    QVBoxLayout* correlationLayout = new QVBoxLayout(correlationTab);
    
    correlationText = new QTextEdit();
    correlationText->setReadOnly(true);
    correlationLayout->addWidget(new QLabel("Корреляционный анализ:"));
    correlationLayout->addWidget(correlationText);
    
    tabWidget->addTab(correlationTab, "Корреляции");
    
    // Вкладка "Выбросы"
    outliersTab = new QWidget();
    QVBoxLayout* outliersLayout = new QVBoxLayout(outliersTab);
    
    outliersTable = new QTableWidget();
    outliersLayout->addWidget(new QLabel("Обнаруженные выбросы:"));
    outliersLayout->addWidget(outliersTable);
    
    tabWidget->addTab(outliersTab, "Выбросы");
    
    mainLayout->addWidget(tabWidget);
    
    // Нижняя панель с кнопками
    QHBoxLayout* bottomLayout = new QHBoxLayout();
    
    backButton = new QPushButton("← Назад к результатам");
    backButton->setStyleSheet("background-color: #607D8B; color: white; border-radius: 10px; padding: 8px 16px;");
    connect(backButton, &QPushButton::clicked, this, &StatisticsWidget::goBack);
    bottomLayout->addWidget(backButton);
    
    bottomLayout->addStretch();
    
    QLabel* exportLabel = new QLabel("Экспорт:");
    bottomLayout->addWidget(exportLabel);
    
    exportFormatCombo = new QComboBox();
    exportFormatCombo->addItem("Текстовый отчет (.txt)", "txt");
    exportFormatCombo->addItem("CSV файл (.csv)", "csv");
    exportFormatCombo->addItem("Markdown (.md)", "md");
    connect(exportFormatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StatisticsWidget::onExportFormatChanged);
    bottomLayout->addWidget(exportFormatCombo);
    
    exportButton = new QPushButton("Экспорт отчета");
    exportButton->setStyleSheet("background-color: #4CAF50; color: white; border-radius: 10px; padding: 8px 16px;");
    connect(exportButton, &QPushButton::clicked, this, &StatisticsWidget::exportReport);
    bottomLayout->addWidget(exportButton);
    
    mainLayout->addLayout(bottomLayout);
}

void StatisticsWidget::showStatistics(const QVector<Cell>& cells) {
    currentCells = cells;
    
    if (cells.isEmpty()) {
        clear();
        QMessageBox::information(this, "Статистика", "Нет данных для анализа");
        return;
    }
    
    Logger::instance().log(QString("StatisticsWidget: Отображение статистики для %1 клеток").arg(cells.size()));
    
    // Выполняем анализ
    currentAnalysis = analyzer.analyzeAllCells(cells);
    
    // Заполняем вкладки
    populateOverviewTab(currentAnalysis);
    populateDetailsTab(currentAnalysis);
    populateDistributionTab(currentAnalysis);
    populateCorrelationTab(currentAnalysis);
    populateOutliersTab(currentAnalysis);
    
    // Переключаемся на первую вкладку
    tabWidget->setCurrentIndex(0);
}

void StatisticsWidget::clear() {
    summaryText->clear();
    overviewTable->setRowCount(0);
    detailsTable->setRowCount(0);
    imageGroupsTable->setRowCount(0);
    distributionText->clear();
    correlationText->clear();
    outliersTable->setRowCount(0);
}

void StatisticsWidget::populateOverviewTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    // Резюме
    summaryText->setPlainText(analysis.summary);
    
    // Основная таблица статистик
    overviewTable->setColumnCount(4);
    overviewTable->setHorizontalHeaderLabels({"Параметр", "Среднее", "Медиана", "Стд. отклонение"});
    overviewTable->setRowCount(3);
    
    // Диаметр в пикселях
    overviewTable->setItem(0, 0, new QTableWidgetItem("Диаметр (пикс)"));
    overviewTable->setItem(0, 1, new QTableWidgetItem(formatStatValue(analysis.diameterPixelsStats.mean)));
    overviewTable->setItem(0, 2, new QTableWidgetItem(formatStatValue(analysis.diameterPixelsStats.median)));
    overviewTable->setItem(0, 3, new QTableWidgetItem(formatStatValue(analysis.diameterPixelsStats.standardDeviation)));
    
    // Диаметр в нанометрах (если доступен)
    if (analysis.diameterNmStats.count > 0) {
        overviewTable->setRowCount(4);
        overviewTable->setItem(1, 0, new QTableWidgetItem("Диаметр (нм)"));
        overviewTable->setItem(1, 1, new QTableWidgetItem(formatStatValue(analysis.diameterNmStats.mean)));
        overviewTable->setItem(1, 2, new QTableWidgetItem(formatStatValue(analysis.diameterNmStats.median)));
        overviewTable->setItem(1, 3, new QTableWidgetItem(formatStatValue(analysis.diameterNmStats.standardDeviation)));
        
        // Площадь
        overviewTable->setItem(2, 0, new QTableWidgetItem("Площадь (пикс²)"));
        overviewTable->setItem(2, 1, new QTableWidgetItem(formatStatValue(analysis.areaStats.mean)));
        overviewTable->setItem(2, 2, new QTableWidgetItem(formatStatValue(analysis.areaStats.median)));
        overviewTable->setItem(2, 3, new QTableWidgetItem(formatStatValue(analysis.areaStats.standardDeviation)));
        
        // Радиус
        overviewTable->setItem(3, 0, new QTableWidgetItem("Радиус (пикс)"));
        overviewTable->setItem(3, 1, new QTableWidgetItem(formatStatValue(analysis.radiusStats.mean)));
        overviewTable->setItem(3, 2, new QTableWidgetItem(formatStatValue(analysis.radiusStats.median)));
        overviewTable->setItem(3, 3, new QTableWidgetItem(formatStatValue(analysis.radiusStats.standardDeviation)));
    } else {
        // Площадь
        overviewTable->setItem(1, 0, new QTableWidgetItem("Площадь (пикс²)"));
        overviewTable->setItem(1, 1, new QTableWidgetItem(formatStatValue(analysis.areaStats.mean)));
        overviewTable->setItem(1, 2, new QTableWidgetItem(formatStatValue(analysis.areaStats.median)));
        overviewTable->setItem(1, 3, new QTableWidgetItem(formatStatValue(analysis.areaStats.standardDeviation)));
        
        // Радиус
        overviewTable->setItem(2, 0, new QTableWidgetItem("Радиус (пикс)"));
        overviewTable->setItem(2, 1, new QTableWidgetItem(formatStatValue(analysis.radiusStats.mean)));
        overviewTable->setItem(2, 2, new QTableWidgetItem(formatStatValue(analysis.radiusStats.median)));
        overviewTable->setItem(2, 3, new QTableWidgetItem(formatStatValue(analysis.radiusStats.standardDeviation)));
    }
    
    overviewTable->resizeColumnsToContents();
    overviewTable->horizontalHeader()->setStretchLastSection(true);
}

void StatisticsWidget::populateDetailsTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    // Детальная таблица
    detailsTable->setColumnCount(8);
    detailsTable->setHorizontalHeaderLabels({
        "Параметр", "Среднее", "Медиана", "Стд. откл.", "Мин", "Макс", "Q1", "Q3"
    });
    
    int rowCount = 3;
    if (analysis.diameterNmStats.count > 0) rowCount = 4;
    
    detailsTable->setRowCount(rowCount);
    
    // Заполняем данные
    auto fillRow = [&](int row, const QString& name, const StatisticsAnalyzer::BasicStatistics& stats) {
        detailsTable->setItem(row, 0, new QTableWidgetItem(name));
        detailsTable->setItem(row, 1, new QTableWidgetItem(formatStatValue(stats.mean)));
        detailsTable->setItem(row, 2, new QTableWidgetItem(formatStatValue(stats.median)));
        detailsTable->setItem(row, 3, new QTableWidgetItem(formatStatValue(stats.standardDeviation)));
        detailsTable->setItem(row, 4, new QTableWidgetItem(formatStatValue(stats.minimum)));
        detailsTable->setItem(row, 5, new QTableWidgetItem(formatStatValue(stats.maximum)));
        detailsTable->setItem(row, 6, new QTableWidgetItem(formatStatValue(stats.q1)));
        detailsTable->setItem(row, 7, new QTableWidgetItem(formatStatValue(stats.q3)));
    };
    
    int currentRow = 0;
    fillRow(currentRow++, "Диаметр (пикс)", analysis.diameterPixelsStats);
    
    if (analysis.diameterNmStats.count > 0) {
        fillRow(currentRow++, "Диаметр (нм)", analysis.diameterNmStats);
    }
    
    fillRow(currentRow++, "Площадь (пикс²)", analysis.areaStats);
    fillRow(currentRow++, "Радиус (пикс)", analysis.radiusStats);
    
    detailsTable->resizeColumnsToContents();
    
    // Таблица по группам изображений
    if (!analysis.imageGroupCounts.isEmpty()) {
        imageGroupsTable->setColumnCount(3);
        imageGroupsTable->setHorizontalHeaderLabels({"Изображение", "Количество клеток", "Средний диаметр"});
        imageGroupsTable->setRowCount(analysis.imageGroupCounts.size());
        
        int row = 0;
        for (auto it = analysis.imageGroupCounts.begin(); it != analysis.imageGroupCounts.end(); ++it, ++row) {
            imageGroupsTable->setItem(row, 0, new QTableWidgetItem(it.key()));
            imageGroupsTable->setItem(row, 1, new QTableWidgetItem(QString::number(it.value())));
            
            if (analysis.imageGroupStats.contains(it.key())) {
                double meanDiameter = analysis.imageGroupStats[it.key()].mean;
                imageGroupsTable->setItem(row, 2, new QTableWidgetItem(formatStatValue(meanDiameter)));
            }
        }
        
        imageGroupsTable->resizeColumnsToContents();
        imageGroupsTable->horizontalHeader()->setStretchLastSection(true);
    }
}

void StatisticsWidget::populateDistributionTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    QString text;
    
    text += "=== АНАЛИЗ РАСПРЕДЕЛЕНИЙ ===\n\n";
    
    text += "РАСПРЕДЕЛЕНИЕ ДИАМЕТРОВ:\n";
    text += QString("• Асимметрия (skewness): %1\n").arg(formatStatValue(analysis.diameterPixelsStats.skewness, 3));
    text += QString("• Эксцесс (kurtosis): %1\n").arg(formatStatValue(analysis.diameterPixelsStats.kurtosis, 3));
    text += QString("• Коэффициент вариации: %1%\n").arg(formatStatValue(analysis.diameterPixelsStats.coefficientOfVariation));
    text += QString("• Межквартильный размах: %1\n\n").arg(formatStatValue(analysis.diameterPixelsStats.iqr));
    
    // Интерпретация асимметрии
    if (analysis.diameterPixelsStats.skewness > 0.5) {
        text += "Распределение смещено вправо - много мелких клеток, мало крупных.\n";
    } else if (analysis.diameterPixelsStats.skewness < -0.5) {
        text += "Распределение смещено влево - много крупных клеток, мало мелких.\n";
    } else {
        text += "Распределение близко к симметричному.\n";
    }
    
    // Интерпретация эксцесса
    if (analysis.diameterPixelsStats.kurtosis > 1.0) {
        text += "Распределение островершинное - значения сконцентрированы вокруг среднего.\n";
    } else if (analysis.diameterPixelsStats.kurtosis < -1.0) {
        text += "Распределение плосковершинное - значения рассеяны.\n";
    } else {
        text += "Распределение близко к нормальному по форме.\n";
    }
    
    text += "\nРАСПРЕДЕЛЕНИЕ ПЛОЩАДЕЙ:\n";
    text += QString("• Асимметрия: %1\n").arg(formatStatValue(analysis.areaStats.skewness, 3));
    text += QString("• Эксцесс: %1\n").arg(formatStatValue(analysis.areaStats.kurtosis, 3));
    text += QString("• Коэффициент вариации: %1%\n").arg(formatStatValue(analysis.areaStats.coefficientOfVariation));
    
    // Гистограмма (текстовая)
    text += "\nГИСТОГРАММА ДИАМЕТРОВ (упрощенная):\n";
    const auto& dist = analysis.diameterDistribution;
    if (!dist.frequencies.isEmpty()) {
        int maxFreq = *std::max_element(dist.frequencies.begin(), dist.frequencies.end());
        
        for (int i = 0; i < dist.frequencies.size(); i++) {
            int barLength = (dist.frequencies[i] * 30) / maxFreq; // Масштабируем до 30 символов
            QString bar = QString(QChar(0x2588)).repeated(barLength);
            text += QString("%1-%2: %3 (%4)\n")
                    .arg(formatStatValue(dist.values[i] - dist.binWidth/2, 0))
                    .arg(formatStatValue(dist.values[i] + dist.binWidth/2, 0))
                    .arg(bar)
                    .arg(dist.frequencies[i]);
        }
    }
    
    distributionText->setPlainText(text);
}

void StatisticsWidget::populateCorrelationTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    QString text;
    
    text += "=== КОРРЕЛЯЦИОННЫЙ АНАЛИЗ ===\n\n";
    
    text += "КОРРЕЛЯЦИЯ ДИАМЕТР-ПЛОЩАДЬ:\n";
    text += QString("• Коэффициент корреляции Пирсона: %1\n\n")
            .arg(formatStatValue(analysis.diameterAreaCorrelation, 3));
    
    // Интерпретация корреляции
    double corr = std::abs(analysis.diameterAreaCorrelation);
    if (corr > 0.9) {
        text += "Очень сильная корреляция - диаметр и площадь тесно связаны.\n";
        text += "Это указывает на то, что клетки имеют преимущественно круглую форму.\n";
    } else if (corr > 0.7) {
        text += "Сильная корреляция - диаметр и площадь хорошо связаны.\n";
        text += "Большинство клеток близки к круглой форме.\n";
    } else if (corr > 0.5) {
        text += "Умеренная корреляция - диаметр и площадь связаны, но не очень тесно.\n";
        text += "Клетки могут иметь различную форму.\n";
    } else if (corr > 0.3) {
        text += "Слабая корреляция - связь между диаметром и площадью слабая.\n";
        text += "Клетки могут значительно отличаться по форме от круглой.\n";
    } else {
        text += "Очень слабая или отсутствующая корреляция.\n";
        text += "Возможны проблемы с обнаружением или измерением клеток.\n";
    }
    
    text += "\nОЖИДАЕМАЯ КОРРЕЛЯЦИЯ:\n";
    text += "Для идеально круглых клеток корреляция диаметр-площадь должна быть близка к 1.0,\n";
    text += "поскольку площадь круга пропорциональна квадрату диаметра (S = π×(d/2)²).\n\n";
    
    text += "ДОПОЛНИТЕЛЬНАЯ ИНФОРМАЦИЯ:\n";
    text += QString("• Количество точек для анализа: %1\n").arg(analysis.diameterPixelsStats.count);
    
    // Коэффициент детерминации
    double r_squared = analysis.diameterAreaCorrelation * analysis.diameterAreaCorrelation;
    text += QString("• Коэффициент детерминации (R²): %1\n").arg(formatStatValue(r_squared, 3));
    text += QString("• Объясненная дисперсия: %1%\n").arg(formatStatValue(r_squared * 100, 1));
    
    correlationText->setPlainText(text);
}

void StatisticsWidget::populateOutliersTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    // Объединяем выбросы по диаметру и площади
    QSet<int> allOutliers;
    for (int idx : analysis.diameterOutliers) {
        allOutliers.insert(idx);
    }
    for (int idx : analysis.areaOutliers) {
        allOutliers.insert(idx);
    }
    
    outliersTable->setColumnCount(6);
    outliersTable->setHorizontalHeaderLabels({
        "№", "Диаметр", "Площадь", "Изображение", "Тип выброса", "Z-score"
    });
    
    QList<int> outliersList(allOutliers.begin(), allOutliers.end());
    std::sort(outliersList.begin(), outliersList.end());
    
    outliersTable->setRowCount(outliersList.size());
    
    // Вычисляем Z-scores для определения степени отклонения
    QVector<double> diameters;
    for (const Cell& cell : currentCells) {
        diameters.append(static_cast<double>(cell.diameter_pixels));
    }
    
    StatisticsAnalyzer::BasicStatistics diamStats = analyzer.calculateBasicStatistics(diameters);
    
    for (int i = 0; i < outliersList.size(); i++) {
        int cellIndex = outliersList[i];
        if (cellIndex >= currentCells.size()) continue;
        
        const Cell& cell = currentCells[cellIndex];
        
        outliersTable->setItem(i, 0, new QTableWidgetItem(QString::number(cellIndex + 1)));
        outliersTable->setItem(i, 1, new QTableWidgetItem(QString::number(cell.diameter_pixels)));
        outliersTable->setItem(i, 2, new QTableWidgetItem(QString::number(cell.area)));
        
        QString imageName = QFileInfo(QString::fromStdString(cell.imagePath)).baseName();
        outliersTable->setItem(i, 3, new QTableWidgetItem(imageName));
        
        // Определяем тип выброса
        QString outlierType;
        if (analysis.diameterOutliers.contains(cellIndex) && analysis.areaOutliers.contains(cellIndex)) {
            outlierType = "Диаметр и площадь";
        } else if (analysis.diameterOutliers.contains(cellIndex)) {
            outlierType = "Диаметр";
        } else {
            outlierType = "Площадь";
        }
        outliersTable->setItem(i, 4, new QTableWidgetItem(outlierType));
        
        // Z-score для диаметра
        double zScore = std::abs(cell.diameter_pixels - diamStats.mean) / diamStats.standardDeviation;
        outliersTable->setItem(i, 5, new QTableWidgetItem(formatStatValue(zScore, 2)));
    }
    
    outliersTable->resizeColumnsToContents();
    outliersTable->horizontalHeader()->setStretchLastSection(true);
}

void StatisticsWidget::exportReport() {
    if (currentCells.isEmpty()) {
        QMessageBox::warning(this, "Экспорт", "Нет данных для экспорта");
        return;
    }
    
    QString format = exportFormatCombo->currentData().toString();
    QString filter;
    QString extension;
    
    if (format == "txt") {
        filter = "Текстовые файлы (*.txt)";
        extension = ".txt";
    } else if (format == "csv") {
        filter = "CSV файлы (*.csv)";
        extension = ".csv";
    } else if (format == "md") {
        filter = "Markdown файлы (*.md)";
        extension = ".md";
    }
    
    QString fileName = QFileDialog::getSaveFileName(
        this,
        "Сохранить отчет",
        QString("statistics_report%1").arg(extension),
        filter
    );
    
    if (fileName.isEmpty()) return;
    
    QString content;
    if (format == "txt") {
        content = analyzer.generateTextReport(currentAnalysis);
    } else if (format == "csv") {
        content = analyzer.generateCSVReport(currentAnalysis);
    } else if (format == "md") {
        content = analyzer.generateMarkdownReport(currentAnalysis);
    }
    
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream(&file);
        stream.setEncoding(QStringConverter::Utf8);
        stream << content;
        file.close();
        
        QMessageBox::information(this, "Экспорт", 
                                QString("Отчет успешно сохранен:\n%1").arg(fileName));
        
        Logger::instance().log(QString("Статистический отчет экспортирован: %1").arg(fileName));
    } else {
        QMessageBox::critical(this, "Ошибка", 
                            QString("Не удалось сохранить файл:\n%1").arg(fileName));
    }
}

void StatisticsWidget::onExportFormatChanged() {
    QString format = exportFormatCombo->currentData().toString();
    
    if (format == "txt") {
        exportButton->setText("Экспорт текстового отчета");
    } else if (format == "csv") {
        exportButton->setText("Экспорт CSV");
    } else if (format == "md") {
        exportButton->setText("Экспорт Markdown");
    }
}

void StatisticsWidget::goBack() {
    emit backToVerification();
}

QString StatisticsWidget::formatStatValue(double value, int precision) {
    return QString::number(value, 'f', precision);
}