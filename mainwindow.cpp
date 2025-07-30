#include "mainwindow.h"
#include "imageprocessor.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QProgressBar>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("Cell Analyzer - Анализатор клеток");
    setMinimumSize(800, 600);
    resize(1024, 768);

    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);

    selectButton = new QPushButton("Выбрать изображения");
    selectButton->setToolTip("Выберите одно или несколько изображений с микроскопа для анализа");
    selectButton->setMinimumHeight(40);
    connect(selectButton, &QPushButton::clicked, this, &MainWindow::selectImages);
    layout->addWidget(selectButton);

    previewGrid = new PreviewGrid(this);
    layout->addWidget(previewGrid);

    verificationWidget = nullptr;

    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    layout->addWidget(progressBar);

    analyzeButton = new QPushButton("Начать анализ");
    analyzeButton->setToolTip("Запустить распознавание клеток на выбранных изображениях");
    analyzeButton->setMinimumHeight(40);
    analyzeButton->setStyleSheet("QPushButton { background-color: #4CAF50; color: white; font-weight: bold; }"
                                "QPushButton:hover { background-color: #45a049; }"
                                "QPushButton:disabled { background-color: #cccccc; }");
    analyzeButton->setEnabled(false);
    connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::startAnalysis);
    layout->addWidget(analyzeButton);

    setCentralWidget(centralWidget);

    connect(previewGrid, &PreviewGrid::pathsChanged, this, &MainWindow::updateAnalysisButtonState);
}

void MainWindow::updateAnalysisButtonState() {
    selectedImagePaths = previewGrid->getPaths();
    analyzeButton->setEnabled(!selectedImagePaths.isEmpty());
}

void MainWindow::selectImages() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Выберите изображения", "", "Images (*.png *.jpg)");
    for (const QString& file : files) {
        previewGrid->addPreview(file);
    }
    // selectedImagePaths обновится через сигнал pathsChanged и слот updateAnalysisButtonState
}

void MainWindow::startAnalysis() {
    if (selectedImagePaths.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Пожалуйста, выберите изображения для анализа");
        return;
    }

    // Отключаем кнопки и показываем прогресс
    selectButton->setEnabled(false);
    analyzeButton->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setFormat("Анализ изображений...");
    progressBar->setRange(0, 0); // Индикатор бесконечной загрузки

    // Обработка изображений
    ImageProcessor processor;
    processor.processImages(selectedImagePaths);

    progressBar->setVisible(false);

    if (processor.getDetectedCells().isEmpty()) {
        QMessageBox::information(this, "Результат", "Клетки не обнаружены на выбранных изображениях");
        selectButton->setEnabled(true);
        analyzeButton->setEnabled(true);
        return;
    }

    if (verificationWidget) {
        verificationWidget->deleteLater();
        verificationWidget = nullptr;
    }
    verificationWidget = new VerificationWidget(processor.getDetectedCells());

    connect(verificationWidget, &VerificationWidget::analysisCompleted, this, [this]() {
        selectButton->setEnabled(true);
        analyzeButton->setEnabled(true);
    });

    // НЕ оборачиваем в QScrollArea, т.к. он уже есть внутри VerificationWidget
    setCentralWidget(verificationWidget);
}

void MainWindow::showVerification() {
    if (!verificationWidget) return;

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(verificationWidget);

    setCentralWidget(scrollArea);
}


MainWindow::~MainWindow() {
    // Очистка ресурсов, если нужно
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    if (!previewGrid) return;  // добавлена проверка

    int width = this->width();
    int previewItemWidth = 120; // ширина превью + отступы
    int maxColumns = width / previewItemWidth;
    if (maxColumns < 1) maxColumns = 1;

    previewGrid->setMaxColumns(maxColumns);
}
