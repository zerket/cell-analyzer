#include "statisticsanalyzer.h"
#include "logger.h"
#include <algorithm>
#include <cmath>
#include <QFileInfo>

StatisticsAnalyzer::StatisticsAnalyzer() {
}

StatisticsAnalyzer::~StatisticsAnalyzer() {
}

StatisticsAnalyzer::ComprehensiveAnalysis StatisticsAnalyzer::analyzeAllCells(const QVector<Cell>& cells) {
    ComprehensiveAnalysis analysis;
    
    if (cells.isEmpty()) {
        Logger::instance().log("StatisticsAnalyzer: Нет клеток для анализа", LogLevel::WARNING);
        return analysis;
    }
    
    Logger::instance().log(QString("StatisticsAnalyzer: Начинаем анализ %1 клеток").arg(cells.size()));
    
    // Извлекаем данные
    QVector<double> diametersPixels = extractDiameters(cells, false);
    QVector<double> diametersNm = extractDiameters(cells, true);
    QVector<double> areas = extractAreas(cells);
    QVector<double> radii = extractRadii(cells);
    
    // Основные статистики
    analysis.diameterPixelsStats = calculateBasicStatistics(diametersPixels);
    analysis.diameterNmStats = calculateBasicStatistics(diametersNm);
    analysis.areaStats = calculateBasicStatistics(areas);
    analysis.radiusStats = calculateBasicStatistics(radii);
    
    // Распределения
    analysis.diameterDistribution = createDistribution(diametersPixels);
    analysis.areaDistribution = createDistribution(areas);
    
    // Группировка по изображениям
    analysis.imageGroupCounts = QMap<QString, int>();
    analysis.imageGroupStats = analyzeByImageGroups(cells);
    
    QMap<QString, QVector<Cell>> imageGroups = groupCellsByImage(cells);
    for (auto it = imageGroups.begin(); it != imageGroups.end(); ++it) {
        analysis.imageGroupCounts[it.key()] = it.value().size();
    }
    
    // Обнаружение выбросов
    analysis.diameterOutliers = detectOutliersIQR(diametersPixels);
    analysis.areaOutliers = detectOutliersIQR(areas);
    
    // Корреляционный анализ
    analysis.diameterAreaCorrelation = calculatePearsonCorrelation(diametersPixels, areas);
    
    // Создаем резюме
    analysis.summary = createSummary(analysis);
    
    Logger::instance().log("StatisticsAnalyzer: Анализ завершен");
    
    return analysis;
}

StatisticsAnalyzer::BasicStatistics StatisticsAnalyzer::calculateBasicStatistics(const QVector<double>& values) {
    BasicStatistics stats;
    
    if (values.isEmpty()) {
        return stats;
    }
    
    stats.count = values.size();
    
    // Сортируем для вычисления медианы и квартилей
    QVector<double> sortedValues = values;
    std::sort(sortedValues.begin(), sortedValues.end());
    
    // Минимум и максимум
    stats.minimum = sortedValues.first();
    stats.maximum = sortedValues.last();
    stats.range = stats.maximum - stats.minimum;
    
    // Среднее значение
    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }
    stats.mean = sum / stats.count;
    
    // Медиана
    stats.median = calculatePercentile(sortedValues, 50.0);
    
    // Квартили
    stats.q1 = calculatePercentile(sortedValues, 25.0);
    stats.q3 = calculatePercentile(sortedValues, 75.0);
    stats.iqr = stats.q3 - stats.q1;
    
    // Дисперсия и стандартное отклонение
    double sumSquaredDiffs = 0.0;
    for (double value : values) {
        double diff = value - stats.mean;
        sumSquaredDiffs += diff * diff;
    }
    stats.variance = sumSquaredDiffs / (stats.count - 1);
    stats.standardDeviation = std::sqrt(stats.variance);
    
    // Коэффициент вариации
    if (stats.mean != 0) {
        stats.coefficientOfVariation = (stats.standardDeviation / stats.mean) * 100.0;
    }
    
    // Асимметрия и эксцесс
    stats.skewness = calculateSkewness(values, stats.mean, stats.standardDeviation);
    stats.kurtosis = calculateKurtosis(values, stats.mean, stats.standardDeviation);
    
    return stats;
}

StatisticsAnalyzer::Distribution StatisticsAnalyzer::createDistribution(const QVector<double>& values, int binCount) {
    Distribution dist;
    
    if (values.isEmpty()) {
        return dist;
    }
    
    dist.binCount = binCount;
    
    // Найдем минимум и максимум
    double minVal = *std::min_element(values.begin(), values.end());
    double maxVal = *std::max_element(values.begin(), values.end());
    
    dist.binWidth = (maxVal - minVal) / binCount;
    
    // Инициализируем массивы
    dist.frequencies.resize(binCount);
    dist.binCenters.resize(binCount);
    dist.values.resize(binCount);
    
    for (int i = 0; i < binCount; i++) {
        dist.frequencies[i] = 0;
        dist.binCenters[i] = minVal + (i + 0.5) * dist.binWidth;
        dist.values[i] = dist.binCenters[i];
    }
    
    // Подсчитываем частоты
    for (double value : values) {
        int binIndex = static_cast<int>((value - minVal) / dist.binWidth);
        if (binIndex >= binCount) binIndex = binCount - 1;
        if (binIndex < 0) binIndex = 0;
        dist.frequencies[binIndex]++;
    }
    
    return dist;
}

StatisticsAnalyzer::BasicStatistics StatisticsAnalyzer::analyzeDiameters(const QVector<Cell>& cells, bool useNanometers) {
    QVector<double> diameters = extractDiameters(cells, useNanometers);
    return calculateBasicStatistics(diameters);
}

StatisticsAnalyzer::BasicStatistics StatisticsAnalyzer::analyzeAreas(const QVector<Cell>& cells) {
    QVector<double> areas = extractAreas(cells);
    return calculateBasicStatistics(areas);
}

StatisticsAnalyzer::BasicStatistics StatisticsAnalyzer::analyzeRadii(const QVector<Cell>& cells) {
    QVector<double> radii = extractRadii(cells);
    return calculateBasicStatistics(radii);
}

QMap<QString, QVector<Cell>> StatisticsAnalyzer::groupCellsByImage(const QVector<Cell>& cells) {
    QMap<QString, QVector<Cell>> groups;
    
    for (const Cell& cell : cells) {
        QString imageName = QFileInfo(QString::fromStdString(cell.imagePath)).baseName();
        groups[imageName].append(cell);
    }
    
    return groups;
}

QMap<QString, StatisticsAnalyzer::BasicStatistics> StatisticsAnalyzer::analyzeByImageGroups(const QVector<Cell>& cells) {
    QMap<QString, BasicStatistics> groupStats;
    QMap<QString, QVector<Cell>> groups = groupCellsByImage(cells);
    
    for (auto it = groups.begin(); it != groups.end(); ++it) {
        QVector<double> diameters = extractDiameters(it.value(), false);
        groupStats[it.key()] = calculateBasicStatistics(diameters);
    }
    
    return groupStats;
}

QVector<int> StatisticsAnalyzer::detectOutliers(const QVector<double>& values, double threshold) {
    return detectOutliersIQR(values, threshold);
}

QVector<int> StatisticsAnalyzer::detectOutliersIQR(const QVector<double>& values, double multiplier) {
    QVector<int> outliers;
    
    if (values.size() < 4) return outliers;
    
    BasicStatistics stats = calculateBasicStatistics(values);
    double lowerBound = stats.q1 - multiplier * stats.iqr;
    double upperBound = stats.q3 + multiplier * stats.iqr;
    
    for (int i = 0; i < values.size(); i++) {
        if (values[i] < lowerBound || values[i] > upperBound) {
            outliers.append(i);
        }
    }
    
    return outliers;
}

QVector<int> StatisticsAnalyzer::detectOutliersZScore(const QVector<double>& values, double threshold) {
    QVector<int> outliers;
    
    if (values.isEmpty()) return outliers;
    
    BasicStatistics stats = calculateBasicStatistics(values);
    
    for (int i = 0; i < values.size(); i++) {
        double zScore = std::abs(values[i] - stats.mean) / stats.standardDeviation;
        if (zScore > threshold) {
            outliers.append(i);
        }
    }
    
    return outliers;
}

double StatisticsAnalyzer::calculateCorrelation(const QVector<double>& x, const QVector<double>& y) {
    return calculatePearsonCorrelation(x, y);
}

double StatisticsAnalyzer::calculatePearsonCorrelation(const QVector<double>& x, const QVector<double>& y) {
    if (x.size() != y.size() || x.isEmpty()) {
        return 0.0;
    }
    
    int n = x.size();
    double sumX = 0.0, sumY = 0.0, sumXY = 0.0, sumX2 = 0.0, sumY2 = 0.0;
    
    for (int i = 0; i < n; i++) {
        sumX += x[i];
        sumY += y[i];
        sumXY += x[i] * y[i];
        sumX2 += x[i] * x[i];
        sumY2 += y[i] * y[i];
    }
    
    double numerator = n * sumXY - sumX * sumY;
    double denominator = std::sqrt((n * sumX2 - sumX * sumX) * (n * sumY2 - sumY * sumY));
    
    if (denominator == 0.0) return 0.0;
    
    return numerator / denominator;
}

double StatisticsAnalyzer::calculateSpearmanCorrelation(const QVector<double>& x, const QVector<double>& y) {
    if (x.size() != y.size() || x.isEmpty()) {
        return 0.0;
    }
    
    // Создаем пары (значение, индекс) для ранжирования
    QVector<QPair<double, int>> xPairs, yPairs;
    for (int i = 0; i < x.size(); i++) {
        xPairs.append(qMakePair(x[i], i));
        yPairs.append(qMakePair(y[i], i));
    }
    
    // Сортируем по значениям
    std::sort(xPairs.begin(), xPairs.end());
    std::sort(yPairs.begin(), yPairs.end());
    
    // Присваиваем ранги
    QVector<double> xRanks(x.size()), yRanks(y.size());
    for (int i = 0; i < x.size(); i++) {
        xRanks[xPairs[i].second] = i + 1;
        yRanks[yPairs[i].second] = i + 1;
    }
    
    // Вычисляем корреляцию Пирсона для рангов
    return calculatePearsonCorrelation(xRanks, yRanks);
}

QString StatisticsAnalyzer::generateTextReport(const ComprehensiveAnalysis& analysis) {
    QString report;
    
    report += "=== СТАТИСТИЧЕСКИЙ АНАЛИЗ КЛЕТОК ===\n\n";
    
    report += "ОБЩАЯ ИНФОРМАЦИЯ:\n";
    report += QString("Общее количество клеток: %1\n").arg(analysis.diameterPixelsStats.count);
    report += QString("Количество изображений: %1\n\n").arg(analysis.imageGroupCounts.size());
    
    report += "ДИАМЕТР (в пикселях):\n";
    report += QString("Среднее: %1\n").arg(formatNumber(analysis.diameterPixelsStats.mean));
    report += QString("Медиана: %1\n").arg(formatNumber(analysis.diameterPixelsStats.median));
    report += QString("Стд. отклонение: %1\n").arg(formatNumber(analysis.diameterPixelsStats.standardDeviation));
    report += QString("Минимум: %1\n").arg(formatNumber(analysis.diameterPixelsStats.minimum));
    report += QString("Максимум: %1\n").arg(formatNumber(analysis.diameterPixelsStats.maximum));
    report += QString("Коэф. вариации: %1%\n\n").arg(formatNumber(analysis.diameterPixelsStats.coefficientOfVariation));
    
    if (analysis.diameterNmStats.count > 0) {
        report += "ДИАМЕТР (в нанометрах):\n";
        report += QString("Среднее: %1 нм\n").arg(formatNumber(analysis.diameterNmStats.mean));
        report += QString("Медиана: %1 нм\n").arg(formatNumber(analysis.diameterNmStats.median));
        report += QString("Стд. отклонение: %1 нм\n\n").arg(formatNumber(analysis.diameterNmStats.standardDeviation));
    }
    
    report += "ПЛОЩАДЬ:\n";
    report += QString("Среднее: %1 пикс²\n").arg(formatNumber(analysis.areaStats.mean));
    report += QString("Медиана: %1 пикс²\n").arg(formatNumber(analysis.areaStats.median));
    report += QString("Стд. отклонение: %1 пикс²\n\n").arg(formatNumber(analysis.areaStats.standardDeviation));
    
    report += "ВЫБРОСЫ:\n";
    report += QString("По диаметру: %1 клеток (%2%)\n")
              .arg(analysis.diameterOutliers.size())
              .arg(formatPercentage(double(analysis.diameterOutliers.size()) / analysis.diameterPixelsStats.count * 100));
    report += QString("По площади: %1 клеток (%2%)\n\n")
              .arg(analysis.areaOutliers.size())
              .arg(formatPercentage(double(analysis.areaOutliers.size()) / analysis.areaStats.count * 100));
    
    report += "КОРРЕЛЯЦИЯ:\n";
    report += QString("Диаметр-Площадь: %1\n\n").arg(formatNumber(analysis.diameterAreaCorrelation, 3));
    
    report += "РЕЗЮМЕ:\n";
    report += analysis.summary;
    
    return report;
}

QString StatisticsAnalyzer::generateCSVReport(const ComprehensiveAnalysis& analysis) {
    QString csv;
    
    csv += "Параметр,Среднее,Медиана,Стд_отклонение,Минимум,Максимум,Количество\n";
    csv += QString("Диаметр_пиксели,%1,%2,%3,%4,%5,%6\n")
           .arg(analysis.diameterPixelsStats.mean)
           .arg(analysis.diameterPixelsStats.median)
           .arg(analysis.diameterPixelsStats.standardDeviation)
           .arg(analysis.diameterPixelsStats.minimum)
           .arg(analysis.diameterPixelsStats.maximum)
           .arg(analysis.diameterPixelsStats.count);
    
    if (analysis.diameterNmStats.count > 0) {
        csv += QString("Диаметр_нанометры,%1,%2,%3,%4,%5,%6\n")
               .arg(analysis.diameterNmStats.mean)
               .arg(analysis.diameterNmStats.median)
               .arg(analysis.diameterNmStats.standardDeviation)
               .arg(analysis.diameterNmStats.minimum)
               .arg(analysis.diameterNmStats.maximum)
               .arg(analysis.diameterNmStats.count);
    }
    
    csv += QString("Площадь,%1,%2,%3,%4,%5,%6\n")
           .arg(analysis.areaStats.mean)
           .arg(analysis.areaStats.median)
           .arg(analysis.areaStats.standardDeviation)
           .arg(analysis.areaStats.minimum)
           .arg(analysis.areaStats.maximum)
           .arg(analysis.areaStats.count);
    
    return csv;
}

QString StatisticsAnalyzer::generateMarkdownReport(const ComprehensiveAnalysis& analysis) {
    QString md;
    
    md += "# Статистический анализ клеток\n\n";
    
    md += "## Общая информация\n\n";
    md += QString("- **Общее количество клеток:** %1\n").arg(analysis.diameterPixelsStats.count);
    md += QString("- **Количество изображений:** %1\n\n").arg(analysis.imageGroupCounts.size());
    
    md += "## Статистики диаметра (пиксели)\n\n";
    md += "| Параметр | Значение |\n";
    md += "|----------|----------|\n";
    md += QString("| Среднее | %1 |\n").arg(formatNumber(analysis.diameterPixelsStats.mean));
    md += QString("| Медиана | %1 |\n").arg(formatNumber(analysis.diameterPixelsStats.median));
    md += QString("| Стандартное отклонение | %1 |\n").arg(formatNumber(analysis.diameterPixelsStats.standardDeviation));
    md += QString("| Минимум | %1 |\n").arg(formatNumber(analysis.diameterPixelsStats.minimum));
    md += QString("| Максимум | %1 |\n").arg(formatNumber(analysis.diameterPixelsStats.maximum));
    md += QString("| Коэффициент вариации | %1% |\n\n").arg(formatNumber(analysis.diameterPixelsStats.coefficientOfVariation));
    
    md += "## Выбросы\n\n";
    md += QString("- **По диаметру:** %1 клеток (%2%)\n")
          .arg(analysis.diameterOutliers.size())
          .arg(formatPercentage(double(analysis.diameterOutliers.size()) / analysis.diameterPixelsStats.count * 100));
    md += QString("- **По площади:** %1 клеток (%2%)\n\n")
          .arg(analysis.areaOutliers.size())
          .arg(formatPercentage(double(analysis.areaOutliers.size()) / analysis.areaStats.count * 100));
    
    md += "## Корреляционный анализ\n\n";
    md += QString("- **Корреляция диаметр-площадь:** %1\n\n").arg(formatNumber(analysis.diameterAreaCorrelation, 3));
    
    md += "## Резюме\n\n";
    md += analysis.summary;
    
    return md;
}

QString StatisticsAnalyzer::formatNumber(double value, int precision) {
    return QString::number(value, 'f', precision);
}

QString StatisticsAnalyzer::formatPercentage(double value, int precision) {
    return QString::number(value, 'f', precision) + "%";
}

double StatisticsAnalyzer::calculateSkewness(const QVector<double>& values, double mean, double stdDev) {
    if (values.size() < 3 || stdDev == 0) return 0.0;
    
    double sum = 0.0;
    for (double value : values) {
        double normalizedValue = (value - mean) / stdDev;
        sum += normalizedValue * normalizedValue * normalizedValue;
    }
    
    return sum / values.size();
}

double StatisticsAnalyzer::calculateKurtosis(const QVector<double>& values, double mean, double stdDev) {
    if (values.size() < 4 || stdDev == 0) return 0.0;
    
    double sum = 0.0;
    for (double value : values) {
        double normalizedValue = (value - mean) / stdDev;
        sum += normalizedValue * normalizedValue * normalizedValue * normalizedValue;
    }
    
    return (sum / values.size()) - 3.0; // Вычитаем 3 для получения избыточного эксцесса
}

double StatisticsAnalyzer::calculatePercentile(const QVector<double>& sortedValues, double percentile) {
    if (sortedValues.isEmpty()) return 0.0;
    
    double index = (percentile / 100.0) * (sortedValues.size() - 1);
    int lowerIndex = static_cast<int>(std::floor(index));
    int upperIndex = static_cast<int>(std::ceil(index));
    
    if (lowerIndex == upperIndex) {
        return sortedValues[lowerIndex];
    }
    
    double weight = index - lowerIndex;
    return sortedValues[lowerIndex] * (1.0 - weight) + sortedValues[upperIndex] * weight;
}

QVector<double> StatisticsAnalyzer::extractDiameters(const QVector<Cell>& cells, bool useNanometers) {
    QVector<double> diameters;
    
    for (const Cell& cell : cells) {
        if (useNanometers && cell.diameter_nm > 0) {
            diameters.append(cell.diameter_nm);
        } else {
            diameters.append(static_cast<double>(cell.diameter_pixels));
        }
    }
    
    return diameters;
}

QVector<double> StatisticsAnalyzer::extractAreas(const QVector<Cell>& cells) {
    QVector<double> areas;
    
    for (const Cell& cell : cells) {
        areas.append(static_cast<double>(cell.area));
    }
    
    return areas;
}

QVector<double> StatisticsAnalyzer::extractRadii(const QVector<Cell>& cells) {
    QVector<double> radii;
    
    for (const Cell& cell : cells) {
        radii.append(static_cast<double>(cell.radius));
    }
    
    return radii;
}

QString StatisticsAnalyzer::createSummary(const ComprehensiveAnalysis& analysis) {
    QString summary;
    
    summary += QString("Проанализировано %1 клеток из %2 изображений. ")
               .arg(analysis.diameterPixelsStats.count)
               .arg(analysis.imageGroupCounts.size());
    
    summary += QString("Средний диаметр составляет %1 пикселей (σ = %2). ")
               .arg(formatNumber(analysis.diameterPixelsStats.mean))
               .arg(formatNumber(analysis.diameterPixelsStats.standardDeviation));
    
    // Анализ распределения
    if (analysis.diameterPixelsStats.skewness > 0.5) {
        summary += "Распределение диаметров смещено вправо (много мелких клеток). ";
    } else if (analysis.diameterPixelsStats.skewness < -0.5) {
        summary += "Распределение диаметров смещено влево (много крупных клеток). ";
    } else {
        summary += "Распределение диаметров близко к симметричному. ";
    }
    
    // Анализ вариабельности
    if (analysis.diameterPixelsStats.coefficientOfVariation < 15) {
        summary += "Клетки довольно однородны по размеру. ";
    } else if (analysis.diameterPixelsStats.coefficientOfVariation > 30) {
        summary += "Клетки сильно различаются по размеру. ";
    } else {
        summary += "Клетки умеренно различаются по размеру. ";
    }
    
    // Анализ выбросов
    double outlierPercent = double(analysis.diameterOutliers.size()) / analysis.diameterPixelsStats.count * 100;
    if (outlierPercent > 10) {
        summary += QString("Обнаружено много выбросов (%1%), что может указывать на наличие артефактов или различных типов клеток. ")
                   .arg(formatNumber(outlierPercent, 1));
    } else if (outlierPercent > 5) {
        summary += QString("Обнаружено умеренное количество выбросов (%1%). ")
                   .arg(formatNumber(outlierPercent, 1));
    } else {
        summary += "Выбросов немного, данные качественные. ";
    }
    
    // Анализ корреляции
    if (analysis.diameterAreaCorrelation > 0.8) {
        summary += "Диаметр и площадь сильно коррелируют, что указывает на преимущественно круглые клетки.";
    } else if (analysis.diameterAreaCorrelation > 0.6) {
        summary += "Диаметр и площадь умеренно коррелируют.";
    } else {
        summary += "Слабая корреляция между диаметром и площадью может указывать на неправильную форму клеток.";
    }
    
    return summary;
}