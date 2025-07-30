#include "mainwindow.h"
#include "imageprocessor.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QProgressBar>
#include <QMessageBox>
#include <QSettings>
#include "settingsmanager.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("Cell Analyzer - Анализатор клеток");
    setMinimumSize(800, 600);
    resize(1024, 768);
    
    // Общий стиль для всех кнопок
    setStyleSheet(
        "QPushButton {"
        "    border-radius: 10px;"
        "    padding: 8px 16px;"
        "    font-size: 14px;"
        "    font-weight: 500;"
        "}"
        "QPushButton:hover {"
        "    background-color: #e0e0e0;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #d0d0d0;"
        "}"
    );

    verificationWidget = nullptr;
    parameterTuningWidget = nullptr;
    
    // Загружаем параметры из настроек
    currentHoughParams = SettingsManager::instance().getHoughParams();
    
    // Создаем главный виджет
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(createMainWidget());
    setCentralWidget(scrollArea);
}

void MainWindow::onPreviewSizeChanged(int value) {
    previewSizeLabel->setText(QString::number(value));
    previewGrid->setPreviewSize(value);
    
    // Сохраняем значение
    SettingsManager::instance().setPreviewSize(value);
    
    // Обновляем количество колонок
    QScrollArea* scrollArea = qobject_cast<QScrollArea*>(centralWidget());
    if (scrollArea && scrollArea->widget() && 
        scrollArea->widget()->findChild<PreviewGrid*>() == previewGrid) {
        int width = this->width() - 60;
        int previewItemWidth = value + 10;
        int maxColumns = width / previewItemWidth;
        if (maxColumns < 1) maxColumns = 1;
        previewGrid->setMaxColumns(maxColumns);
    }
}

void MainWindow::onParametersConfirmed(const ParameterTuningWidget::HoughParams& params) {
    currentHoughParams = params;
    // Сохраняем параметры
    SettingsManager::instance().setHoughParams(params);
    
    // Удаляем виджет настройки параметров
    if (parameterTuningWidget) {
        parameterTuningWidget->hide();
        parameterTuningWidget->deleteLater();
        parameterTuningWidget = nullptr;
    }
    
    // Обработка изображений с заданными параметрами
    ImageProcessor processor;
    processor.processImages(selectedImagePaths, params);

    if (processor.getDetectedCells().isEmpty()) {
        QMessageBox::information(this, "Результат", "Клетки не обнаружены на выбранных изображениях");
        
        // Возвращаемся к главному экрану
        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(createMainWidget());
        setCentralWidget(scrollArea);
        return;
    }

    if (verificationWidget) {
        verificationWidget->deleteLater();
        verificationWidget = nullptr;
    }
    verificationWidget = new VerificationWidget(processor.getDetectedCells());

    connect(verificationWidget, &VerificationWidget::analysisCompleted, this, [this]() {
        // Возвращаемся к главному экрану после анализа
        QScrollArea* scrollArea = new QScrollArea(this);
        scrollArea->setWidgetResizable(true);
        scrollArea->setWidget(createMainWidget());
        setCentralWidget(scrollArea);
    });

    // НЕ оборачиваем в QScrollArea, т.к. он уже есть внутри VerificationWidget
    setCentralWidget(verificationWidget);
}

void MainWindow::updateAnalysisButtonState() {
    selectedImagePaths = previewGrid->getPaths();
    analyzeButton->setEnabled(!selectedImagePaths.isEmpty());
}

QWidget* MainWindow::createMainWidget() {
    centralWidgetContainer = new QWidget();
    centralLayout = new QVBoxLayout(centralWidgetContainer);
    centralLayout->setSpacing(10);
    centralLayout->setContentsMargins(20, 20, 20, 20);
    
    // Кнопка выбора изображений
    selectButton = new QPushButton("Выбрать изображения");
    selectButton->setToolTip("Выберите одно или несколько изображений с микроскопа для анализа");
    selectButton->setFixedWidth(250);
    selectButton->setMinimumHeight(50);
    selectButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    font-size: 16px;"
        "    font-weight: bold;"
        "    border-radius: 10px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
    );
    connect(selectButton, &QPushButton::clicked, this, &MainWindow::selectImages);
    
    // Область для превью
    previewGrid = new PreviewGrid(this);
    int savedSize = SettingsManager::instance().getPreviewSize();
    previewGrid->setPreviewSize(savedSize);
    connect(previewGrid, &PreviewGrid::pathsChanged, this, &MainWindow::updateAnalysisButtonState);
    
    // Прогресс бар
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);
    
    // Нижний тулбар
    toolbarWidget = new QWidget();
    toolbarWidget->setVisible(false);
    toolbarWidget->setMaximumHeight(150);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(0, 10, 0, 10);
    
    // Ползунок размера превью
    QLabel* sizeLabel = new QLabel("Размер превью:");
    toolbarLayout->addWidget(sizeLabel);
    
    previewSizeSlider = new QSlider(Qt::Horizontal);
    previewSizeSlider->setRange(100, 500);
    previewSizeSlider->setValue(savedSize);
    previewSizeSlider->setMaximumWidth(200);
    connect(previewSizeSlider, &QSlider::valueChanged, this, &MainWindow::onPreviewSizeChanged);
    toolbarLayout->addWidget(previewSizeSlider);
    
    previewSizeLabel = new QLabel(QString::number(savedSize));
    previewSizeLabel->setMinimumWidth(30);
    toolbarLayout->addWidget(previewSizeLabel);
    
    toolbarLayout->addSpacing(20);
    
    // Кнопка очистки
    clearButton = new QPushButton("Очистить");
    clearButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f44336;"
        "    color: white;"
        "    border-radius: 10px;"
        "    padding: 8px 20px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #d32f2f;"
        "}"
    );
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearImages);
    toolbarLayout->addWidget(clearButton);
    
    // Кнопка добавления изображений
    addImagesButton = new QPushButton("Добавить изображения");
    addImagesButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border-radius: 10px;"
        "    padding: 8px 20px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #45a049;"
        "}"
    );
    connect(addImagesButton, &QPushButton::clicked, this, &MainWindow::selectImages);
    toolbarLayout->addWidget(addImagesButton);
    
    toolbarLayout->addStretch();
    
    // Кнопка анализа
    analyzeButton = new QPushButton("Начать анализ");
    analyzeButton->setMinimumHeight(40);
    analyzeButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #03A9F4;"
        "    color: white;"
        "    font-weight: bold;"
        "    border-radius: 10px;"
        "    padding: 8px 30px;"
        "}"
        "QPushButton:hover {"
        "    background-color: #0288D1;"
        "}"
        "QPushButton:disabled {"
        "    background-color: #B0BEC5;"
        "}"
    );
    analyzeButton->setEnabled(false);
    connect(analyzeButton, &QPushButton::clicked, this, &MainWindow::startAnalysis);
    toolbarLayout->addWidget(analyzeButton);
    
    setupInitialState();
    
    return centralWidgetContainer;
}

void MainWindow::setupInitialState() {
    // Начальное состояние: только кнопка выбора по центру
    centralLayout->addStretch();
    
    QHBoxLayout* centerLayout = new QHBoxLayout();
    centerLayout->addStretch();
    centerLayout->addWidget(selectButton);
    centerLayout->addStretch();
    centralLayout->addLayout(centerLayout);
    
    centralLayout->addStretch();
    
    selectButton->show(); // Убедимся, что кнопка видна
    previewGrid->hide();
    toolbarWidget->hide();
    progressBar->hide();
}

void MainWindow::setupWithImagesState() {
    // Состояние с изображениями
    // Очищаем layout
    QLayoutItem* item;
    while ((item = centralLayout->takeAt(0)) != nullptr) {
        if (item->widget() && item->widget() != selectButton && 
            item->widget() != previewGrid && item->widget() != progressBar && 
            item->widget() != toolbarWidget) {
            delete item->widget();
        }
        delete item;
    }
    
    selectButton->hide();
    
    centralLayout->addWidget(previewGrid);
    centralLayout->addWidget(progressBar);
    centralLayout->addWidget(toolbarWidget);
    
    previewGrid->show();
    toolbarWidget->show();
}

void MainWindow::clearImages() {
    // Очищаем все изображения
    previewGrid->clearAll();
    
    // Возвращаемся к начальному состоянию
    setupInitialState();
}

void MainWindow::selectImages() {
    QStringList files = QFileDialog::getOpenFileNames(this, "Выберите изображения", "", "Images (*.png *.jpg *.jpeg *.bmp)");
    if (!files.isEmpty()) {
        for (const QString& file : files) {
            previewGrid->addPreview(file);
        }
        setupWithImagesState();
    }
}

void MainWindow::startAnalysis() {
    if (selectedImagePaths.isEmpty()) {
        QMessageBox::warning(this, "Предупреждение", "Пожалуйста, выберите изображения для анализа");
        return;
    }

    // Если выбрано только одно изображение, показываем экран настройки параметров
    if (selectedImagePaths.size() == 1) {
        if (parameterTuningWidget) {
            parameterTuningWidget->deleteLater();
            parameterTuningWidget = nullptr;
        }
        
        parameterTuningWidget = new ParameterTuningWidget(selectedImagePaths.first(), this);
        connect(parameterTuningWidget, &ParameterTuningWidget::parametersConfirmed,
                this, &MainWindow::onParametersConfirmed);
        
        setCentralWidget(parameterTuningWidget);
        return;
    }
    
    // Для нескольких изображений используем параметры по умолчанию
    onParametersConfirmed(currentHoughParams);
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

    if (!previewGrid) return;
    
    // Проверяем, что мы на главном экране
    QScrollArea* scrollArea = qobject_cast<QScrollArea*>(centralWidget());
    if (!scrollArea || !scrollArea->widget() || 
        scrollArea->widget()->findChild<PreviewGrid*>() != previewGrid) {
        return;
    }

    int width = this->width() - 60; // учитываем отступы и скроллбар
    int previewItemWidth = previewGrid->getPreviewSize() + 10; // размер превью + отступы
    int maxColumns = width / previewItemWidth;
    if (maxColumns < 1) maxColumns = 1;

    previewGrid->setMaxColumns(maxColumns);
}
