#include <QApplication>
#include <QtConcurrent/QtConcurrent>
#include "mainwindow.h"
#include "logger.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    INIT_LOGGER();
    
    // Инициализируем логирование
    LOG_INFO("==================================================");
    LOG_INFO("CellAnalyzer application started");
    LOG_INFO("==================================================");
    try {
        MainWindow window;
        window.show();
        return app.exec();
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Fatal error: %1").arg(e.what()));
        return 1;
    } catch (...) {
        LOG_ERROR("Fatal error: Unknown exception");
        return 1;
    }
}
