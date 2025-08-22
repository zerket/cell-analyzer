#ifndef STATISTICSWIDGET_H
#define STATISTICSWIDGET_H

#include <QWidget>
#include <QTextEdit>
#include <QTabWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTableWidget>
#include <QComboBox>
#include "statisticsanalyzer.h"
#include "cell.h"

class StatisticsWidget : public QWidget {
    Q_OBJECT

public:
    explicit StatisticsWidget(QWidget *parent = nullptr);
    ~StatisticsWidget();
    
    void showStatistics(const QVector<Cell>& cells);
    void clear();

signals:
    void backToVerification();

private slots:
    void exportReport();
    void onExportFormatChanged();
    void goBack();

private:
    void setupUI();
    void populateOverviewTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis);
    void populateDetailsTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis);
    void populateDistributionTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis);
    void populateCorrelationTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis);
    void populateOutliersTab(const StatisticsAnalyzer::ComprehensiveAnalysis& analysis);
    
    QWidget* createStatisticsTable(const StatisticsAnalyzer::BasicStatistics& stats, const QString& title);
    QWidget* createImageGroupsTable(const QMap<QString, int>& groupCounts, 
                                   const QMap<QString, StatisticsAnalyzer::BasicStatistics>& groupStats);
    
    QString formatStatValue(double value, int precision = 2);
    
private:
    QTabWidget* tabWidget;
    
    // Вкладки
    QWidget* overviewTab;
    QWidget* detailsTab;
    QWidget* distributionTab;
    QWidget* correlationTab;
    QWidget* outliersTab;
    
    // Элементы интерфейса
    QTextEdit* summaryText;
    QTableWidget* overviewTable;
    QTableWidget* detailsTable;
    QTableWidget* imageGroupsTable;
    QTextEdit* distributionText;
    QTextEdit* correlationText;
    QTableWidget* outliersTable;
    
    // Кнопки управления
    QPushButton* backButton;
    QPushButton* exportButton;
    QComboBox* exportFormatCombo;
    
    // Данные
    StatisticsAnalyzer::ComprehensiveAnalysis currentAnalysis;
    QVector<Cell> currentCells;
    StatisticsAnalyzer analyzer;
};

#endif // STATISTICSWIDGET_H