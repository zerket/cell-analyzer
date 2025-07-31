#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QMutex>

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void log(const QString& message, const QString& level = "INFO") {
        QMutexLocker locker(&mutex);
        
        if (!file.isOpen()) {
            file.setFileName("cell_analyzer_debug.log");
            if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
                qDebug() << "Failed to open log file";
                return;
            }
        }

        QTextStream stream(&file);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        stream << timestamp << " [" << level << "] " << message << Qt::endl;
        stream.flush();
        
        // Также выводим в консоль для отладки
        qDebug() << timestamp << "[" << level << "]" << message;
    }

    void info(const QString& message) { log(message, "INFO"); }
    void warning(const QString& message) { log(message, "WARN"); }
    void error(const QString& message) { log(message, "ERROR"); }
    void debug(const QString& message) { log(message, "DEBUG"); }

    ~Logger() {
        if (file.isOpen()) {
            file.close();
        }
    }

private:
    Logger() {}
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    QFile file;
    QMutex mutex;
};

#define LOG_INFO(msg) Logger::instance().info(msg)
#define LOG_WARNING(msg) Logger::instance().warning(msg)
#define LOG_ERROR(msg) Logger::instance().error(msg)
#define LOG_DEBUG(msg) Logger::instance().debug(msg)

#endif // LOGGER_H