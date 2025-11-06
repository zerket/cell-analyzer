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

    setWindowTitle("Cell Analyzer - Анализатор клеток");
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
    parameterTuningWidget = nullptr;
    statisticsWidget = nullptr;

    // Загружаем параметры из настроек (последний выбранный пресет)
    QString lastPreset = SettingsManager::instance().getLastSelectedPreset();
    currentHoughParams = SettingsManager::instance().getPresetByName(lastPreset);

    // Создаем главный виджет напрямую без внешнего QScrollArea
    setCentralWidget(createMainWidget());
}


void MainWindow::onParametersConfirmed(const ParameterTuningWidget::HoughParams& params) {
    LOG_INFO("MainWindow::onParametersConfirmed() called");
    
    currentHoughParams = params;
    LOG_INFO("Parameters confirmed and saved in preset");
    
    LOG_INFO("Cleaning up parameter tuning widget");
    // Удаляем виджет настройки параметров
    if (parameterTuningWidget) {
        parameterTuningWidget->hide();
        parameterTuningWidget->deleteLater();
        parameterTuningWidget = nullptr;
        LOG_INFO("Parameter tuning widget cleaned up");
    }
    
    LOG_INFO("Creating ImageProcessor");
    // Обработка изображений с заданными параметрами
    ImageProcessor* processor = nullptr;
    try {
        processor = new ImageProcessor();
        LOG_INFO("ImageProcessor created successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to create ImageProcessor: %1").arg(e.what()));
        return;
    }
    
    LOG_INFO(QString("Processing %1 images").arg(selectedImagePaths.size()));
    try {
        processor->processImages(selectedImagePaths, params);
        LOG_INFO("Images processed successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to process images: %1").arg(e.what()));
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

    LOG_INFO("Cleaning up old verification widget");
    if (verificationWidget) {
        verificationWidget->deleteLater();
        verificationWidget = nullptr;
    }
    
    LOG_INFO("Creating copy of detected cells");
    // Создаем копию вектора cells перед удалением processor
    QVector<Cell> detectedCells = processor->getDetectedCells();
    LOG_INFO(QString("Copied %1 cells").arg(detectedCells.size()));
    
    delete processor;
    LOG_INFO("ImageProcessor deleted");
    
    LOG_INFO("Creating VerificationWidget");
    try {
        verificationWidget = new VerificationWidget(detectedCells);
        LOG_INFO("VerificationWidget created successfully");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Failed to create VerificationWidget: %1").arg(e.what()));
        return;
    }

    LOG_INFO("Connecting signals");
    connect(verificationWidget, &VerificationWidget::analysisCompleted, this, [this]() {
        LOG_INFO("Analysis completed, returning to main screen");
        // Возвращаемся к главному экрану после анализа
        setCentralWidget(createMainWidget());
    });
    
    connect(verificationWidget, &VerificationWidget::statisticsRequested, this, &MainWindow::showStatistics);

    LOG_INFO("Setting VerificationWidget as central widget");
    // НЕ оборачиваем в QScrollArea, т.к. он уже есть внутри VerificationWidget
    setCentralWidget(verificationWidget);
    LOG_INFO("onParametersConfirmed() completed successfully");
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

    // Создаем scroll area для превью (будет добавлена позже в setupWithImagesState)
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

    // Добавляем scroll area с previewGrid (растягивается на все доступное пространство)
    if (centralLayout->indexOf(previewScrollArea) == -1) {
        centralLayout->addWidget(previewScrollArea, 1); // stretch factor = 1
    }

    // Прогресс бар
    if (centralLayout->indexOf(progressBar) == -1) {
        centralLayout->addWidget(progressBar);
    }

    // Toolbar всегда внизу без stretch (фиксированная позиция)
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

    // Всегда показываем экран настройки параметров, используя первое изображение для предпросмотра
    if (parameterTuningWidget) {
        parameterTuningWidget->deleteLater();
        parameterTuningWidget = nullptr;
    }
    
    parameterTuningWidget = new ParameterTuningWidget(selectedImagePaths.first(), this);
    connect(parameterTuningWidget, &ParameterTuningWidget::parametersConfirmed,
            this, &MainWindow::onParametersConfirmed);
    
    setCentralWidget(parameterTuningWidget);
}

void MainWindow::showVerification() {
    if (!verificationWidget) {
        Logger::instance().log("showVerification: verificationWidget is null", LogLevel::ERROR);
        return;
    }

    Logger::instance().log("Возврат к окну проверки результатов");

    // Извлекаем текущий центральный виджет БЕЗ удаления (QScrollArea со статистикой)
    QWidget* oldCentral = takeCentralWidget();
    if (oldCentral) {
        Logger::instance().log("Удаляем QScrollArea со статистикой");
        oldCentral->deleteLater();
    }

    // НЕ оборачиваем в QScrollArea, т.к. он уже есть внутри VerificationWidget
    setCentralWidget(verificationWidget);
    Logger::instance().log("VerificationWidget установлен как центральный виджет");
}


MainWindow::~MainWindow() {
    // Очистка ресурсов, если нужно
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);

    if (!previewGrid || !previewScrollArea) return;

    // Проверяем, что мы на главном экране (centralWidget == centralWidgetContainer)
    if (centralWidget() != centralWidgetContainer) {
        return;
    }

    // Проверяем, что previewScrollArea видим (значит мы в режиме с изображениями)
    if (!previewScrollArea->isVisible()) {
        return;
    }

    int width = previewScrollArea->viewport()->width() - 20; // учитываем отступы
    int previewItemWidth = previewGrid->getPreviewSize() + 10; // размер превью + отступы
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
        // Ищем файл в нескольких возможных местах
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
            readmeContent = "# CellAnalyzer\n\nНе удалось загрузить файл README.md\n\n"
                          "Приложение для автоматического обнаружения и анализа клеток на микроскопических изображениях.\n\n"
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
        // Обнуляем указатель при удалении виджета
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
