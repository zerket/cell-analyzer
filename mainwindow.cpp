#include "mainwindow.h"
#include "imageprocessor.h"
#include "thememanager.h"
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileDialog>
#include <QProgressBar>
#include <QMessageBox>
#include <QSettings>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QDialog>
#include <QTextEdit>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include "settingsmanager.h"
#include "logger.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {

    setWindowTitle("Cell Analyzer - Анализатор клеток (YOLO)");
    setMinimumSize(1200, 800);
    resize(1200, 800);

    // Создаем меню
    setupMenuBar();

    // Инициализируем тему
    ThemeManager& themeManager = ThemeManager::instance();
    connect(&themeManager, &ThemeManager::themeChanged, this, [](ThemeManager::Theme theme) {
        Logger::instance().log("Тема изменена на: " + QString(theme == ThemeManager::Theme::Dark ? "темную" : "светлую"));
    });

    verificationWidget = nullptr;
    statisticsWidget = nullptr;

    // Создаем главный виджет
    setCentralWidget(createMainWidget());
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

    // Создаем scroll area для превью
    previewScrollArea = new QScrollArea(this);
    previewScrollArea->setWidgetResizable(true);
    previewScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    previewScrollArea->setVisible(false);

    // Область для превью
    previewGrid = new PreviewGrid(this);
    previewGrid->setPreviewSize(200);
    previewScrollArea->setWidget(previewGrid);
    connect(previewGrid, &PreviewGrid::pathsChanged, this, &MainWindow::updateAnalysisButtonState);

    // Прогресс бар
    progressBar = new QProgressBar(this);
    progressBar->setVisible(false);
    progressBar->setTextVisible(true);

    // Нижний тулбар
    toolbarWidget = new QWidget();
    toolbarWidget->setVisible(false);
    toolbarWidget->setMaximumHeight(80);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbarWidget);
    toolbarLayout->setContentsMargins(0, 5, 0, 5);

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
    analyzeButton = new QPushButton("Начать анализ (YOLO)");
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

    selectButton->show();
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
            item->widget() != previewScrollArea && item->widget() != progressBar &&
            item->widget() != toolbarWidget) {
            delete item->widget();
        }
        delete item;
    }

    selectButton->hide();

    // Добавляем scroll area с previewGrid
    if (centralLayout->indexOf(previewScrollArea) == -1) {
        centralLayout->addWidget(previewScrollArea, 1);
    }

    // Прогресс бар
    if (centralLayout->indexOf(progressBar) == -1) {
        centralLayout->addWidget(progressBar);
    }

    // Toolbar всегда внизу
    if (centralLayout->indexOf(toolbarWidget) == -1) {
        centralLayout->addWidget(toolbarWidget);
    }

    previewScrollArea->show();
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

    LOG_INFO(QString("Starting YOLO analysis for %1 images").arg(selectedImagePaths.size()));

    // Создаем ImageProcessor
    ImageProcessor* processor = nullptr;
    try {
        processor = new ImageProcessor();
        LOG_INFO("ImageProcessor created successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to create ImageProcessor: %1").arg(e.what()));
        QMessageBox::critical(this, "Ошибка", QString("Не удалось создать обработчик изображений: %1").arg(e.what()));
        return;
    }

    // Обработка изображений с параметрами YOLO по умолчанию
    ImageProcessor::YoloParams params;  // Default: conf=0.25, iou=0.7, minArea=500

    LOG_INFO(QString("Processing %1 images with YOLO").arg(selectedImagePaths.size()));

    try {
        processor->processImages(selectedImagePaths, params);
        LOG_INFO("Images processed successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to process images: %1").arg(e.what()));
        QMessageBox::critical(this, "Ошибка", QString("Ошибка обработки изображений: %1").arg(e.what()));
        delete processor;
        return;
    }

    LOG_INFO(QString("Detected %1 cells").arg(processor->getDetectedCells().size()));

    if (processor->getDetectedCells().isEmpty()) {
        LOG_WARNING("No cells detected");
        QMessageBox::information(this, "Результат", "Клетки не обнаружены на выбранных изображениях");
        delete processor;

        // Возвращаемся к главному экрану
        LOG_INFO("Returning to main screen");
        setCentralWidget(createMainWidget());
        return;
    }

    LOG_INFO("Creating copy of detected cells");
    // Создаем копию вектора cells перед удалением processor
    QVector<Cell> detectedCells = processor->getDetectedCells();
    LOG_INFO(QString("Copied %1 cells").arg(detectedCells.size()));

    delete processor;
    LOG_INFO("ImageProcessor deleted");

    // Очищаем старый verification widget
    if (verificationWidget) {
        verificationWidget->deleteLater();
        verificationWidget = nullptr;
    }

    LOG_INFO("Creating VerificationWidget");
    try {
        verificationWidget = new VerificationWidget(detectedCells);
        LOG_INFO("VerificationWidget created successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to create VerificationWidget: %1").arg(e.what()));
        QMessageBox::critical(this, "Ошибка", QString("Не удалось создать окно верификации: %1").arg(e.what()));
        return;
    }

    LOG_INFO("Connecting signals");
    connect(verificationWidget, &VerificationWidget::analysisCompleted, this, [this]() {
        LOG_INFO("Analysis completed, returning to main screen");
        setCentralWidget(createMainWidget());
    });

    connect(verificationWidget, &VerificationWidget::statisticsRequested, this, &MainWindow::showStatistics);

    LOG_INFO("Setting VerificationWidget as central widget");
    setCentralWidget(verificationWidget);
    LOG_INFO("Analysis started successfully");
}

void MainWindow::showVerification() {
    if (!verificationWidget) {
        Logger::instance().log("showVerification: verificationWidget is null", LogLevel::ERROR);
        return;
    }

    Logger::instance().log("Возврат к окну проверки результатов");

    // Извлекаем текущий центральный виджет БЕЗ удаления
    QWidget* oldCentral = takeCentralWidget();
    if (oldCentral) {
        Logger::instance().log("Удаляем старый центральный виджет");
        oldCentral->deleteLater();
    }

    setCentralWidget(verificationWidget);
    Logger::instance().log("VerificationWidget установлен как центральный виджет");
}


MainWindow::~MainWindow() {
    // Очистка ресурсов
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    if (!previewGrid || !previewScrollArea) return;

    // Проверяем, что мы на главном экране
    if (centralWidget() != centralWidgetContainer) {
        return;
    }

    // Проверяем, что previewScrollArea видим
    if (!previewScrollArea->isVisible()) {
        return;
    }

    int width = previewScrollArea->viewport()->width() - 20;
    int previewItemWidth = previewGrid->getPreviewSize() + 10;
    int maxColumns = width / previewItemWidth;
    if (maxColumns < 1) maxColumns = 1;

    previewGrid->setMaxColumns(maxColumns);
}

void MainWindow::setupMenuBar() {
    QMenuBar* menuBar = this->menuBar();

    // Меню "Вид"
    QMenu* viewMenu = menuBar->addMenu("Вид");

    // Действие переключения темы
    QAction* toggleThemeAction = new QAction("Переключить тему", this);
    toggleThemeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(toggleThemeAction, &QAction::triggered, []() {
        ThemeManager::instance().toggleTheme();
    });
    viewMenu->addAction(toggleThemeAction);

    // Подменю выбора темы
    QMenu* themeMenu = viewMenu->addMenu("Выбрать тему");

    QAction* lightThemeAction = new QAction("Светлая тема", this);
    connect(lightThemeAction, &QAction::triggered, []() {
        ThemeManager::instance().setTheme(ThemeManager::Theme::Light);
    });
    themeMenu->addAction(lightThemeAction);

    QAction* darkThemeAction = new QAction("Темная тема", this);
    connect(darkThemeAction, &QAction::triggered, []() {
        ThemeManager::instance().setTheme(ThemeManager::Theme::Dark);
    });
    themeMenu->addAction(darkThemeAction);

    // Меню "Справка"
    QMenu* helpMenu = menuBar->addMenu("Справка");

    // Действие "О программе"
    QAction* aboutAction = new QAction("О программе", this);
    aboutAction->setShortcut(QKeySequence("F1"));
    connect(aboutAction, &QAction::triggered, this, [this]() {
        // Читаем содержимое README.md
        QStringList possiblePaths = {
            QCoreApplication::applicationDirPath() + "/README.md",
            QCoreApplication::applicationDirPath() + "/../README.md",
            QCoreApplication::applicationDirPath() + "/../../README.md",
            QCoreApplication::applicationDirPath() + "/../../../README.md"
        };

        QString readmeContent;
        bool found = false;

        for (const QString& path : possiblePaths) {
            QFile readmeFile(path);
            if (readmeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&readmeFile);
                in.setEncoding(QStringConverter::Utf8);
                readmeContent = in.readAll();
                readmeFile.close();
                found = true;
                LOG_INFO(QString("README.md загружен из: %1").arg(path));
                break;
            }
        }

        if (!found) {
            readmeContent = "# CellAnalyzer (YOLO Edition)\n\nНе удалось загрузить файл README.md\n\n"
                          "Приложение для автоматического обнаружения и анализа клеток на микроскопических изображениях.\n\n"
                          "Использует YOLOv8 для детекции клеток.\n\n"
                          "Попробованные пути:\n";
            for (const QString& path : possiblePaths) {
                readmeContent += "- " + path + "\n";
            }
            LOG_WARNING("README.md не найден ни в одном из стандартных путей");
        }

        // Создаем диалог
        QDialog* aboutDialog = new QDialog(this);
        aboutDialog->setWindowTitle("О программе CellAnalyzer");
        aboutDialog->resize(800, 600);

        QVBoxLayout* layout = new QVBoxLayout(aboutDialog);

        // Текстовый виджет с содержимым README
        QTextEdit* textEdit = new QTextEdit(aboutDialog);
        textEdit->setReadOnly(true);
        textEdit->setMarkdown(readmeContent);
        layout->addWidget(textEdit);

        // Кнопка закрытия
        QPushButton* closeButton = new QPushButton("Закрыть", aboutDialog);
        closeButton->setStyleSheet(
            "QPushButton {"
            "    background-color: #2196F3;"
            "    color: white;"
            "    border-radius: 10px;"
            "    padding: 8px 20px;"
            "}"
            "QPushButton:hover {"
            "    background-color: #1976D2;"
            "}"
        );
        connect(closeButton, &QPushButton::clicked, aboutDialog, &QDialog::accept);

        QHBoxLayout* buttonLayout = new QHBoxLayout();
        buttonLayout->addStretch();
        buttonLayout->addWidget(closeButton);
        buttonLayout->addStretch();
        layout->addLayout(buttonLayout);

        aboutDialog->exec();
        aboutDialog->deleteLater();
    });
    helpMenu->addAction(aboutAction);
}

void MainWindow::showStatistics() {
    if (!verificationWidget) {
        Logger::instance().log("Нет данных для статистического анализа", LogLevel::WARNING);
        return;
    }

    QVector<Cell> cells = verificationWidget->getVerifiedCells();
    if (cells.isEmpty()) {
        QMessageBox::information(this, "Статистика", "Нет обнаруженных клеток для анализа");
        return;
    }

    // Извлекаем текущий центральный виджет БЕЗ удаления
    QWidget* oldCentral = takeCentralWidget();
    Logger::instance().log(QString("Извлечен центральный виджет: %1").arg(oldCentral ? "да" : "нет"));

    if (!statisticsWidget) {
        statisticsWidget = new StatisticsWidget(this);
        connect(statisticsWidget, &StatisticsWidget::backToVerification,
                this, &MainWindow::onBackFromStatistics);
        connect(statisticsWidget, &QObject::destroyed, this, [this]() {
            statisticsWidget = nullptr;
            Logger::instance().log("StatisticsWidget удален, указатель обнулен");
        });
    }

    statisticsWidget->showStatistics(cells);

    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setWidget(statisticsWidget);
    setCentralWidget(scrollArea);

    Logger::instance().log("Открыт статистический анализ");
}

void MainWindow::onBackFromStatistics() {
    Logger::instance().log("onBackFromStatistics: Обработка возврата из статистики");

    try {
        showVerification();
        Logger::instance().log("onBackFromStatistics: Успешно вернулись к проверке результатов");
    } catch (const std::exception& e) {
        Logger::instance().log(QString("onBackFromStatistics: Исключение: %1").arg(e.what()), LogLevel::ERROR);
    } catch (...) {
        Logger::instance().log("onBackFromStatistics: Неизвестное исключение", LogLevel::ERROR);
    }
}
