#include "settingsmanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include "logger.h"

SettingsManager& SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager() {
    ensureSettingsDirectory();
    loadSettings();
}

void SettingsManager::ensureSettingsDirectory() {
    // Настройки хранятся рядом с .exe файлом
    QString appDir = QCoreApplication::applicationDirPath();
    QDir dir(appDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QString SettingsManager::getSettingsPath() const {
    // Настройки хранятся рядом с .exe файлом
    QString appDir = QCoreApplication::applicationDirPath();
    return QDir(appDir).filePath(m_settingsFile);
}

void SettingsManager::saveSettings() {
    QJsonObject root;

    // Сохраняем размер превью
    root["previewSize"] = m_previewSize;

    // Сохраняем пороги статистики
    root["statisticsMinThreshold"] = m_statisticsMinThreshold;
    root["statisticsMaxThreshold"] = m_statisticsMaxThreshold;

    // Сохраняем коэффициент
    root["coefficient"] = m_coefficient;

    // Сохраняем тему
    root["Theme"] = m_theme;

    // Сохраняем цвета клеток
    root["cellHighlightColor"] = m_cellHighlightColor;
    root["cellSelectionColor"] = m_cellSelectionColor;

    // Сохраняем флаг игнорирования пограничных клеток
    root["ignoreBorderCells"] = m_ignoreBorderCells;

    // Сохраняем порог видимости клетки
    root["cellVisibilityThreshold"] = m_cellVisibilityThreshold;

    // Сохраняем фильтры по диаметру
    root["minCellDiameter"] = m_minCellDiameter;
    root["maxCellDiameter"] = m_maxCellDiameter;

    // Записываем в файл
    QJsonDocument doc(root);
    QFile file(getSettingsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        LOG_INFO(QString("Settings saved to: %1").arg(getSettingsPath()));
    } else {
        LOG_ERROR(QString("Failed to save settings to: %1").arg(getSettingsPath()));
    }
}

void SettingsManager::loadSettings() {
    QFile file(getSettingsPath());
    if (!file.exists()) {
        LOG_INFO("Settings file not found, using defaults");
        saveSettings(); // Создаем файл с настройками по умолчанию
        return;
    }

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject root = doc.object();

            // Загружаем размер превью
            if (root.contains("previewSize")) {
                m_previewSize = root["previewSize"].toInt(150);
            }

            // Загружаем пороги статистики
            if (root.contains("statisticsMinThreshold")) {
                m_statisticsMinThreshold = root["statisticsMinThreshold"].toDouble(50.0);
            }
            if (root.contains("statisticsMaxThreshold")) {
                m_statisticsMaxThreshold = root["statisticsMaxThreshold"].toDouble(100.0);
            }

            // Загружаем коэффициент
            if (root.contains("coefficient")) {
                m_coefficient = root["coefficient"].toDouble(0.0);
            }

            // Загружаем тему
            if (root.contains("Theme")) {
                m_theme = root["Theme"].toString("Dark");
            }

            // Загружаем цвета клеток
            if (root.contains("cellHighlightColor")) {
                m_cellHighlightColor = root["cellHighlightColor"].toString("#00FF00");
            }
            if (root.contains("cellSelectionColor")) {
                m_cellSelectionColor = root["cellSelectionColor"].toString("#FF0000");
            }

            // Загружаем флаг игнорирования пограничных клеток
            if (root.contains("ignoreBorderCells")) {
                m_ignoreBorderCells = root["ignoreBorderCells"].toBool(false);
            }

            // Загружаем порог видимости клетки
            if (root.contains("cellVisibilityThreshold")) {
                m_cellVisibilityThreshold = root["cellVisibilityThreshold"].toInt(85);
            }

            // Загружаем фильтры по диаметру
            if (root.contains("minCellDiameter")) {
                m_minCellDiameter = root["minCellDiameter"].toInt(30);
            }
            if (root.contains("maxCellDiameter")) {
                m_maxCellDiameter = root["maxCellDiameter"].toInt(160);
            }

            // Сохраняем весь объект для общих настроек
            m_settings = root;

            LOG_INFO(QString("Settings loaded from: %1").arg(getSettingsPath()));
        }
    } else {
        LOG_ERROR(QString("Failed to load settings from: %1").arg(getSettingsPath()));
    }
}

void SettingsManager::setPreviewSize(int size) {
    m_previewSize = size;
    saveSettings();
}

void SettingsManager::setStatisticsMinThreshold(double threshold) {
    m_statisticsMinThreshold = threshold;
    saveSettings();
}

void SettingsManager::setStatisticsMaxThreshold(double threshold) {
    m_statisticsMaxThreshold = threshold;
    saveSettings();
}

void SettingsManager::setCoefficient(double coefficient) {
    m_coefficient = coefficient;
    saveSettings();
    LOG_INFO(QString("Coefficient updated: %1 μm/px").arg(coefficient));
}

void SettingsManager::setTheme(const QString& theme) {
    m_theme = theme;
    saveSettings();
}

QVariant SettingsManager::getValue(const QString& key, const QVariant& defaultValue) const {
    const_cast<SettingsManager*>(this)->loadSettings();

    QStringList keys = key.split('/');
    QJsonObject current = m_settings;

    for (int i = 0; i < keys.size() - 1; i++) {
        if (current.contains(keys[i]) && current[keys[i]].isObject()) {
            current = current[keys[i]].toObject();
        } else {
            return defaultValue;
        }
    }

    QString lastKey = keys.last();
    if (current.contains(lastKey)) {
        QJsonValue value = current[lastKey];
        if (value.isBool()) return value.toBool();
        if (value.isDouble()) return value.toDouble();
        if (value.isString()) return value.toString();
        if (value.isNull()) return QVariant();
    }

    return defaultValue;
}

void SettingsManager::setValue(const QString& key, const QVariant& value) {
    loadSettings();

    QStringList keys = key.split('/');
    QJsonObject* current = &m_settings;

    // Navigate to the correct location
    for (int i = 0; i < keys.size() - 1; i++) {
        if (!current->contains(keys[i]) || !(*current)[keys[i]].isObject()) {
            (*current)[keys[i]] = QJsonObject();
        }
        QJsonObject temp = (*current)[keys[i]].toObject();
        (*current)[keys[i]] = temp;
        current = &temp;
    }

    // Set the value
    QString lastKey = keys.last();
    if (value.type() == QVariant::Bool) {
        (*current)[lastKey] = value.toBool();
    } else if (value.type() == QVariant::Double || value.type() == QVariant::Int) {
        (*current)[lastKey] = value.toDouble();
    } else {
        (*current)[lastKey] = value.toString();
    }

    saveSettings();
}

void SettingsManager::setCellHighlightColor(const QString& color) {
    m_cellHighlightColor = color;
    saveSettings();
}

void SettingsManager::setCellSelectionColor(const QString& color) {
    m_cellSelectionColor = color;
    saveSettings();
}

void SettingsManager::setIgnoreBorderCells(bool ignore) {
    m_ignoreBorderCells = ignore;
    saveSettings();
}

void SettingsManager::setCellVisibilityThreshold(int threshold) {
    m_cellVisibilityThreshold = threshold;
    saveSettings();
    LOG_INFO(QString("Cell visibility threshold updated: %1%").arg(threshold));
}

void SettingsManager::setMinCellDiameter(int diameter) {
    m_minCellDiameter = diameter;
    saveSettings();
    LOG_INFO(QString("Min cell diameter updated: %1 px").arg(diameter));
}

void SettingsManager::setMaxCellDiameter(int diameter) {
    m_maxCellDiameter = diameter;
    saveSettings();
    LOG_INFO(QString("Max cell diameter updated: %1 px").arg(diameter));
}
