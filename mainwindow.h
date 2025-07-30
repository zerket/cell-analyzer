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
#include "parametertuningwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectImages();
    void startAnalysis();
    void showVerification();
    void updateAnalysisButtonState();
    void onParametersConfirmed(const ParameterTuningWidget::HoughParams& params);
    void onPreviewSizeChanged(int value);
    void clearImages();

private:
    PreviewGrid* previewGrid;
    VerificationWidget* verificationWidget;
    ParameterTuningWidget* parameterTuningWidget;
    QPushButton* analyzeButton;
    QPushButton* selectButton;
    QProgressBar* progressBar;
    QSlider* previewSizeSlider;
    QLabel* previewSizeLabel;
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

protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // MAINWINDOW_H
