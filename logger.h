// logger.h - IMPROVED VERSION
#ifndef LOGGER_H
#define LOGGER_H

#include <QString>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDebug>
#include <QMutex>
#include <QMutexLocker>
#include <QDir>
#include <QFileInfo>
#include <memory>
#include <atomic>

enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARNING = 2,
    ERROR = 3,
    CRITICAL = 4
};

class Logger {
public:
    static Logger& instance() {
        static Logger instance;
        return instance;
    }

    void setLogLevel(LogLevel level) {
        m_logLevel = level;
    }

    void setMaxFileSize(qint64 bytes) {
        QMutexLocker locker(&m_mutex);
        m_maxFileSize = bytes;
    }

    void setMaxBackupFiles(int count) {
        QMutexLocker locker(&m_mutex);
        m_maxBackupFiles = count;
    }

    void log(const QString& message, LogLevel level = LogLevel::INFO, 
             const QString& file = "", int line = 0) {
        
        // Check log level
        if (level < m_logLevel) {
            return;
        }

        QMutexLocker locker(&m_mutex);
        
        if (!ensureLogFile()) {
            return;
        }

        QTextStream stream(&m_file);
        QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        QString levelStr = levelToString(level);
        
        QString logEntry = QString("%1 [%2]").arg(timestamp).arg(levelStr);
        
        // Add file and line info for debug builds
        #ifdef QT_DEBUG
        if (!file.isEmpty()) {
            QFileInfo fi(file);
            logEntry += QString(" [%1:%2]").arg(fi.fileName()).arg(line);
        }
        #endif
        
        logEntry += QString(" %1").arg(message);
        
        stream << logEntry << Qt::endl;
        stream.flush();
        
        // Also output to console in debug mode
        #ifdef QT_DEBUG
        qDebug().noquote() << logEntry;
        #endif
        
        // Increment write counter for rotation check
        m_writeCounter++;
        if (m_writeCounter % 100 == 0) { // Check every 100 writes
            checkRotation();
        }
    }

    void debug(const QString& message, const QString& file = "", int line = 0) { 
        log(message, LogLevel::DEBUG, file, line); 
    }
    
    void info(const QString& message, const QString& file = "", int line = 0) { 
        log(message, LogLevel::INFO, file, line); 
    }
    
    void warning(const QString& message, const QString& file = "", int line = 0) { 
        log(message, LogLevel::WARNING, file, line); 
    }
    
    void error(const QString& message, const QString& file = "", int line = 0) { 
        log(message, LogLevel::ERROR, file, line); 
    }
    
    void critical(const QString& message, const QString& file = "", int line = 0) { 
        log(message, LogLevel::CRITICAL, file, line); 
    }

    ~Logger() {
        QMutexLocker locker(&m_mutex);
        if (m_file.isOpen()) {
            m_file.close();
        }
    }

private:
    Logger() : m_logLevel(LogLevel::INFO), 
               m_maxFileSize(10 * 1024 * 1024), // 10 MB default
               m_maxBackupFiles(5),
               m_writeCounter(0) {
        ensureLogDirectory();
    }
    
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    bool ensureLogFile() {
        if (!m_file.isOpen()) {
            QString logPath = getLogFilePath();
            m_file.setFileName(logPath);
            
            if (!m_file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
                qDebug() << "Failed to open log file:" << logPath;
                return false;
            }
        }
        return true;
    }

    void ensureLogDirectory() {
        QDir logDir(getLogDirectory());
        if (!logDir.exists()) {
            logDir.mkpath(".");
        }
    }

    QString getLogDirectory() const {
        // Use application directory for logs
        return QDir::currentPath() + "/logs";
    }

    QString getLogFilePath() const {
        return getLogDirectory() + "/cell_analyzer.log";
    }

    void checkRotation() {
        if (m_file.size() > m_maxFileSize) {
            rotateLogFiles();
        }
    }

    void rotateLogFiles() {
        m_file.close();
        
        QString basePath = getLogFilePath();
        
        // Delete oldest backup if we're at max
        QString oldestBackup = QString("%1.%2").arg(basePath).arg(m_maxBackupFiles);
        if (QFile::exists(oldestBackup)) {
            QFile::remove(oldestBackup);
        }
        
        // Rotate existing backups
        for (int i = m_maxBackupFiles - 1; i > 0; --i) {
            QString oldName = QString("%1.%2").arg(basePath).arg(i);
            QString newName = QString("%1.%2").arg(basePath).arg(i + 1);
            
            if (QFile::exists(oldName)) {
                QFile::rename(oldName, newName);
            }
        }
        
        // Rename current log to .1
        QFile::rename(basePath, QString("%1.1").arg(basePath));
        
        // Open new log file
        ensureLogFile();
        
        info("Log file rotated");
    }

    QString levelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARN";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRIT";
            default:                 return "UNKNOWN";
        }
    }

    QFile m_file;
    QMutex m_mutex;
    LogLevel m_logLevel;
    qint64 m_maxFileSize;
    int m_maxBackupFiles;
    std::atomic<int> m_writeCounter;
};

// Convenient macros with file and line info
#define LOG_DEBUG(msg) Logger::instance().debug(msg, __FILE__, __LINE__)
#define LOG_INFO(msg) Logger::instance().info(msg, __FILE__, __LINE__)
#define LOG_WARNING(msg) Logger::instance().warning(msg, __FILE__, __LINE__)
#define LOG_ERROR(msg) Logger::instance().error(msg, __FILE__, __LINE__)
#define LOG_CRITICAL(msg) Logger::instance().critical(msg, __FILE__, __LINE__)

// Set log level at startup
#ifdef QT_DEBUG
    #define INIT_LOGGER() Logger::instance().setLogLevel(LogLevel::DEBUG)
#else
    #define INIT_LOGGER() Logger::instance().setLogLevel(LogLevel::INFO)
#endif

#endif // LOGGER_H