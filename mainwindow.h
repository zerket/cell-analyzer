#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QScrollArea>
#include <QWidget>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include "previewgrid.h"
#include "verificationwidget.h"
#include "statisticswidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectImages();
    void startAnalysis();
    void showVerification();
    void showStatistics();
    void updateAnalysisButtonState();
    void clearImages();
    void onBackFromStatistics();

private:
    PreviewGrid* previewGrid;
    VerificationWidget* verificationWidget;
    StatisticsWidget* statisticsWidget;
    QPushButton* analyzeButton;
    QPushButton* selectButton;
    QProgressBar* progressBar;
    QStringList selectedImagePaths;

    QWidget* centralWidgetContainer;
    QVBoxLayout* centralLayout;
    QScrollArea* previewScrollArea;

    QPushButton* clearButton;
    QPushButton* addImagesButton;
    QWidget* toolbarWidget;

private:
    QWidget* createMainWidget();
    void setupInitialState();
    void setupWithImagesState();
    void setupMenuBar();

protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // MAINWINDOW_H
