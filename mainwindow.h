#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QStringList>
#include <QScrollArea>
#include <QWidget>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QPushButton>
#include "previewgrid.h"
#include "verificationwidget.h"

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

private:
    PreviewGrid* previewGrid;
    VerificationWidget* verificationWidget;
    QPushButton* analyzeButton;
    QPushButton* selectButton;
    QProgressBar* progressBar;
    QStringList selectedImagePaths;

    QWidget* centralWidgetContainer;
    QVBoxLayout* centralLayout;

protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // MAINWINDOW_H
