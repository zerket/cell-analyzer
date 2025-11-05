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
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include "previewgrid.h"
#include "verificationwidget.h"
#include "parametertuningwidget.h"
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
    void onParametersConfirmed(const ParameterTuningWidget::HoughParams& params);
    void clearImages();
    void onBackFromStatistics();

private:
    PreviewGrid* previewGrid;
    VerificationWidget* verificationWidget;
    ParameterTuningWidget* parameterTuningWidget;
    StatisticsWidget* statisticsWidget;
    QPushButton* analyzeButton;
    QPushButton* selectButton;
    QProgressBar* progressBar;
    QStringList selectedImagePaths;
    ParameterTuningWidget::HoughParams currentHoughParams;

    QWidget* centralWidgetContainer;
    QVBoxLayout* centralLayout;
    
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
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    void processDroppedFiles(const QStringList& filePaths);
    bool isImageFile(const QString& filePath) const;
};

#endif // MAINWINDOW_H
