#include "settingsmanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>
#include <QCoreApplication>

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
    
    // Сохраняем параметры HoughCircles
    root["houghParams"] = houghParamsToJson(m_houghParams);
    
    // Сохраняем размер превью
    root["previewSize"] = m_previewSize;
    
    // Сохраняем коэффициент μm/pixel
    root["nmPerPixel"] = m_nmPerPixel;

    // Сохраняем пороги статистики
    root["statisticsMinThreshold"] = m_statisticsMinThreshold;
    root["statisticsMaxThreshold"] = m_statisticsMaxThreshold;

    // Записываем в файл
    QJsonDocument doc(root);
    QFile file(getSettingsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        qDebug() << "Settings saved to:" << getSettingsPath();
    } else {
        qDebug() << "Failed to save settings to:" << getSettingsPath();
    }
}

void SettingsManager::loadSettings() {
    QFile file(getSettingsPath());
    if (!file.exists()) {
        qDebug() << "Settings file not found, using defaults";
        saveSettings(); // Создаем файл с настройками по умолчанию
        return;
    }
    
    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();
        
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject root = doc.object();
            
            // Загружаем параметры HoughCircles
            if (root.contains("houghParams") && root["houghParams"].isObject()) {
                m_houghParams = jsonToHoughParams(root["houghParams"].toObject());
            }
            
            // Загружаем размер превью
            if (root.contains("previewSize")) {
                m_previewSize = root["previewSize"].toInt(150);
            }
            
            // Загружаем коэффициент μm/pixel
            if (root.contains("nmPerPixel")) {
                m_nmPerPixel = root["nmPerPixel"].toDouble(0.0);
            }

            // Загружаем пороги статистики
            if (root.contains("statisticsMinThreshold")) {
                m_statisticsMinThreshold = root["statisticsMinThreshold"].toDouble(50.0);
            }
            if (root.contains("statisticsMaxThreshold")) {
                m_statisticsMaxThreshold = root["statisticsMaxThreshold"].toDouble(100.0);
            }

            // Сохраняем весь объект для общих настроек
            m_settings = root;
            
            qDebug() << "Settings loaded from:" << getSettingsPath();
        }
    } else {
        qDebug() << "Failed to load settings from:" << getSettingsPath();
    }
}

void SettingsManager::setHoughParams(const ImageProcessor::HoughParams& params) {
    m_houghParams = params;
    saveSettings();
}

void SettingsManager::setPreviewSize(int size) {
    m_previewSize = size;
    saveSettings();
}

void SettingsManager::setNmPerPixel(double coefficient) {
    m_nmPerPixel = coefficient;
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

QJsonObject SettingsManager::houghParamsToJson(const ImageProcessor::HoughParams& params) const {
    QJsonObject json;
    json["dp"] = params.dp;
    json["minDist"] = params.minDist;
    json["param1"] = params.param1;
    json["param2"] = params.param2;
    json["minRadius"] = params.minRadius;
    json["maxRadius"] = params.maxRadius;
    return json;
}

ImageProcessor::HoughParams SettingsManager::jsonToHoughParams(const QJsonObject& json) const {
    ImageProcessor::HoughParams params;
    params.dp = json["dp"].toDouble(1.0);
    params.minDist = json["minDist"].toDouble(30.0);
    params.param1 = json["param1"].toDouble(90.0);
    params.param2 = json["param2"].toDouble(50.0);
    params.minRadius = json["minRadius"].toInt(30);
    params.maxRadius = json["maxRadius"].toInt(150);
    return params;
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

QJsonObject SettingsManager::getPresets() const {
    loadSettings();
    if (m_settings.contains("presets") && m_settings["presets"].isObject()) {
        return m_settings["presets"].toObject();
    }
    return QJsonObject();
}

void SettingsManager::setPresets(const QJsonObject& presets) {
    loadSettings();
    m_settings["presets"] = presets;
    saveSettings();
}

QString SettingsManager::getLastSelectedPreset() const {
    return getValue("lastSelectedPreset", "По умолчанию").toString();
}

void SettingsManager::setLastSelectedPreset(const QString& presetName) {
    setValue("lastSelectedPreset", presetName);
}