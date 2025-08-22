#ifndef STATISTICSANALYZER_H
#define STATISTICSANALYZER_H

#include <QVector>
#include <QString>
#include <QMap>
#include "cell.h"

class StatisticsAnalyzer {
public:
    struct BasicStatistics {
        double mean = 0.0;
        double median = 0.0;
        double standardDeviation = 0.0;
        double variance = 0.0;
        double minimum = 0.0;
        double maximum = 0.0;
        double range = 0.0;
        int count = 0;
        
        // Квартили
        double q1 = 0.0;  // 25-й процентиль
        double q3 = 0.0;  // 75-й процентиль
        double iqr = 0.0; // Межквартильный размах
        
        // Дополнительные статистики
        double skewness = 0.0;    // Асимметрия
        double kurtosis = 0.0;    // Эксцесс
        double coefficientOfVariation = 0.0; // Коэффициент вариации
    };
    
    struct Distribution {
        QVector<double> values;
        QVector<int> frequencies;
        QVector<double> binCenters;
        int binCount = 10;
        double binWidth = 0.0;
    };
    
    struct ComprehensiveAnalysis {
        BasicStatistics diameterStats;    // Только диаметры в микрометрах
        BasicStatistics areaStats;       // Площади в мкм²
        
        Distribution diameterDistribution;
        
        QMap<QString, int> imageGroupCounts;  // Количество клеток по изображениям
        QMap<QString, BasicStatistics> imageGroupStats; // Статистики по изображениям
        
        // Выбросы
        QVector<int> diameterOutliers;  // Индексы клеток-выбросов по диаметру (мкм)
        
        QString summary;  // Текстовое резюме анализа
    };

    StatisticsAnalyzer();
    ~StatisticsAnalyzer();
    
    // Основные функции анализа
    ComprehensiveAnalysis analyzeAllCells(const QVector<Cell>& cells);
    BasicStatistics calculateBasicStatistics(const QVector<double>& values);
    Distribution createDistribution(const QVector<double>& values, int binCount = 10);
    
    // Статистики отдельных параметров (только микрометры)
    BasicStatistics analyzeDiameters(const QVector<Cell>& cells);
    BasicStatistics analyzeAreas(const QVector<Cell>& cells);
    
    // Группировка по изображениям
    QMap<QString, QVector<Cell>> groupCellsByImage(const QVector<Cell>& cells);
    QMap<QString, BasicStatistics> analyzeByImageGroups(const QVector<Cell>& cells);
    
    // Обнаружение выбросов
    QVector<int> detectOutliers(const QVector<double>& values, double threshold = 1.5);
    QVector<int> detectOutliersIQR(const QVector<double>& values, double multiplier = 1.5);
    QVector<int> detectOutliersZScore(const QVector<double>& values, double threshold = 2.0);
    
    // Корреляционный анализ
    double calculateCorrelation(const QVector<double>& x, const QVector<double>& y);
    double calculatePearsonCorrelation(const QVector<double>& x, const QVector<double>& y);
    double calculateSpearmanCorrelation(const QVector<double>& x, const QVector<double>& y);
    
    // Экспорт результатов
    QString generateTextReport(const ComprehensiveAnalysis& analysis);
    QString generateCSVReport(const ComprehensiveAnalysis& analysis);
    QString generateMarkdownReport(const ComprehensiveAnalysis& analysis);
    
    // Форматирование чисел
    static QString formatNumber(double value, int precision = 2);
    static QString formatPercentage(double value, int precision = 1);
    
    // Вспомогательные функции для статистик
    static double calculateSkewness(const QVector<double>& values, double mean, double stdDev);
    static double calculateKurtosis(const QVector<double>& values, double mean, double stdDev);
    static double calculatePercentile(const QVector<double>& sortedValues, double percentile);
    
private:
    // Вспомогательные функции
    QVector<double> extractDiameters(const QVector<Cell>& cells);
    QVector<double> extractAreas(const QVector<Cell>& cells);
    
    QString createSummary(const ComprehensiveAnalysis& analysis);
    QString analyzeDistributionShape(const BasicStatistics& stats);
    QString compareWithNormalDistribution(const QVector<double>& values);
    
    // Нормализация данных
    QVector<double> normalizeValues(const QVector<double>& values);
    QVector<double> standardizeValues(const QVector<double>& values);
};

#endif // STATISTICSANALYZER_H