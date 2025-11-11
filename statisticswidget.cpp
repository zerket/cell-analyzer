#include "statisticswidget.h"
#include "logger.h"
#include "settingsmanager.h"
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
    overviewTable->setColumnCount(2);
    overviewTable->setHorizontalHeaderLabels({"Параметр", "Значение"});
    overviewTable->setRowCount(4);

    // Медиана
    overviewTable->setItem(0, 0, new QTableWidgetItem("Медиана (мкм)"));
    overviewTable->setItem(0, 1, new QTableWidgetItem(formatStatValue(analysis.diameterStats.median)));

    // Среднее
    overviewTable->setItem(1, 0, new QTableWidgetItem("Среднее (мкм)"));
    overviewTable->setItem(1, 1, new QTableWidgetItem(formatStatValue(analysis.diameterStats.mean)));

    // Процент < 50 мкм (из расчетов в StatisticsAnalyzer)
    overviewTable->setItem(2, 0, new QTableWidgetItem("% < 50 мкм"));
    overviewTable->setItem(2, 1, new QTableWidgetItem(
        formatStatValue(analysis.diameterStats.percentBelow50) + "%" +
        QString(" (%1 клеток)").arg(analysis.diameterStats.countBelow50)
    ));

    // Процент > 100 мкм (из расчетов в StatisticsAnalyzer)
    overviewTable->setItem(3, 0, new QTableWidgetItem("% > 100 мкм"));
    overviewTable->setItem(3, 1, new QTableWidgetItem(
        formatStatValue(analysis.diameterStats.percentAbove100) + "%" +
        QString(" (%1 клеток)").arg(analysis.diameterStats.countAbove100)
    ));

    overviewTable->resizeColumnsToContents();
    overviewTable->horizontalHeader()->setStretchLastSection(true);
}

void StatisticsWidget::populateDetailsTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    // Детальная таблица
    detailsTable->setColumnCount(8);
    detailsTable->setHorizontalHeaderLabels({
        "Параметр", "Среднее", "Медиана", "Стд. откл.", "Мин", "Макс", "Q1", "Q3"
    });
    
    detailsTable->setRowCount(2);
    
    // Заполняем данные (только микрометры)
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
    
    fillRow(0, "Диаметр (мкм)", analysis.diameterStats);
    fillRow(1, "Площадь (мкм²)", analysis.areaStats);
    
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
    text += QString("• Асимметрия (skewness): %1\n").arg(formatStatValue(analysis.diameterStats.skewness, 3));
    text += QString("• Эксцесс (kurtosis): %1\n").arg(formatStatValue(analysis.diameterStats.kurtosis, 3));
    text += QString("• Коэффициент вариации: %1%\n").arg(formatStatValue(analysis.diameterStats.coefficientOfVariation));
    text += QString("• Межквартильный размах: %1\n\n").arg(formatStatValue(analysis.diameterStats.iqr));
    
    // Интерпретация асимметрии
    if (analysis.diameterStats.skewness > 0.5) {
        text += "Распределение смещено вправо - много мелких клеток, мало крупных.\n";
    } else if (analysis.diameterStats.skewness < -0.5) {
        text += "Распределение смещено влево - много крупных клеток, мало мелких.\n";
    } else {
        text += "Распределение близко к симметричному.\n";
    }
    
    // Интерпретация эксцесса
    if (analysis.diameterStats.kurtosis > 1.0) {
        text += "Распределение островершинное - значения сконцентрированы вокруг среднего.\n";
    } else if (analysis.diameterStats.kurtosis < -1.0) {
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
    if (!dist.frequencies.isEmpty() && dist.binWidth > 0) {
        int maxFreq = *std::max_element(dist.frequencies.begin(), dist.frequencies.end());
        
        for (int i = 0; i < dist.frequencies.size(); i++) {
            if (dist.frequencies[i] > 0) { // Показываем только непустые бины
                int barLength = (dist.frequencies[i] * 30) / maxFreq; // Масштабируем до 30 символов
                QString bar = QString(QChar(0x2588)).repeated(barLength);
                double binStart = dist.values[i] - dist.binWidth/2;
                double binEnd = dist.values[i] + dist.binWidth/2;
                text += QString("%1-%2 мкм: %3 (%4)\n")
                        .arg(formatStatValue(binStart, 2))
                        .arg(formatStatValue(binEnd, 2))
                        .arg(bar)
                        .arg(dist.frequencies[i]);
            }
        }
    } else {
        text += "Нет данных для построения гистограммы (возможно, не задан масштаб)\n";
    }
    
    distributionText->setPlainText(text);
}

void StatisticsWidget::populateCorrelationTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    QString text;
    
    text += "=== АНАЛИЗ РАЗМЕРОВ ===\n\n";
    
    text += "СООТНОШЕНИЕ ДИАМЕТР-ПЛОЩАДЬ:\n";
    text += "Площади рассчитаны на основе диаметров, предполагая круглую форму клеток:\n";
    text += "Площадь = π × (диаметр/2)²\n\n";
    
    text += QString("• Средний диаметр: %1 мкм\n").arg(formatStatValue(analysis.diameterStats.mean));
    text += QString("• Средняя площадь: %2 мкм²\n").arg(formatStatValue(analysis.areaStats.mean));
    
    if (analysis.diameterStats.mean > 0) {
        double expectedArea = M_PI * (analysis.diameterStats.mean / 2.0) * (analysis.diameterStats.mean / 2.0);
        text += QString("• Ожидаемая площадь для среднего диаметра: %1 мкм²\n").arg(formatStatValue(expectedArea));
    }
    
    text += "\nВАРИАЦИИ РАЗМЕРОВ:\n";
    text += QString("• Коэффициент вариации диаметров: %1%\n").arg(formatStatValue(analysis.diameterStats.coefficientOfVariation));
    text += QString("• Коэффициент вариации площадей: %1%\n").arg(formatStatValue(analysis.areaStats.coefficientOfVariation));
    
    text += "\nДОПОЛНИТЕЛЬНАЯ ИНФОРМАЦИЯ:\n";
    text += QString("• Количество клеток: %1\n").arg(analysis.diameterStats.count);
    text += QString("• Диапазон диаметров: %1 - %2 мкм\n")
            .arg(formatStatValue(analysis.diameterStats.minimum))
            .arg(formatStatValue(analysis.diameterStats.maximum));
    text += QString("• Диапазон площадей: %1 - %2 мкм²\n")
            .arg(formatStatValue(analysis.areaStats.minimum))
            .arg(formatStatValue(analysis.areaStats.maximum));
    
    correlationText->setPlainText(text);
}

void StatisticsWidget::populateOutliersTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis) {
    if (currentCells.isEmpty()) {
        outliersTable->setRowCount(0);
        return;
    }
    
    // Используем только выбросы по диаметру
    QSet<int> allOutliers;
    for (int idx : analysis.diameterOutliers) {
        if (idx >= 0 && idx < currentCells.size()) {
            allOutliers.insert(idx);
        }
    }
    
    outliersTable->setColumnCount(5);
    outliersTable->setHorizontalHeaderLabels({
        "№", "Диаметр (мкм)", "Площадь (мкм²)", "Изображение", "Z-score"
    });
    
    QList<int> outliersList(allOutliers.begin(), allOutliers.end());
    std::sort(outliersList.begin(), outliersList.end());
    
    outliersTable->setRowCount(outliersList.size());
    
    // Используем статистику диаметров из анализа
    StatisticsAnalyzer::BasicStatistics diamStats = analysis.diameterStats;
    
    int rowIndex = 0;
    for (int i = 0; i < outliersList.size(); i++) {
        int cellIndex = outliersList[i];
        if (cellIndex < 0 || cellIndex >= currentCells.size()) {
            continue;
        }
        
        const Cell& cell = currentCells[cellIndex];
        
        outliersTable->setItem(rowIndex, 0, new QTableWidgetItem(QString::number(cellIndex + 1)));
        outliersTable->setItem(rowIndex, 1, new QTableWidgetItem(formatStatValue(cell.diameter_um)));
        // Вычисляем площадь в мкм²
        double area_um2 = M_PI * (cell.diameter_um / 2.0) * (cell.diameter_um / 2.0);
        outliersTable->setItem(rowIndex, 2, new QTableWidgetItem(formatStatValue(area_um2)));
        
        QString imageName = QFileInfo(QString::fromStdString(cell.imagePath)).baseName();
        outliersTable->setItem(rowIndex, 3, new QTableWidgetItem(imageName));
        
        // Z-score для диаметра (в микрометрах)
        if (diamStats.standardDeviation > 0) {
            double zScore = std::abs(cell.diameter_um - diamStats.mean) / diamStats.standardDeviation;
            outliersTable->setItem(rowIndex, 4, new QTableWidgetItem(formatStatValue(zScore, 2)));
        } else {
            outliersTable->setItem(rowIndex, 4, new QTableWidgetItem("N/A"));
        }
        
        rowIndex++;
    }
    
    // Корректируем размер таблицы до фактического количества строк
    outliersTable->setRowCount(rowIndex);
    
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