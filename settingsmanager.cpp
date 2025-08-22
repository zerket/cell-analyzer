#include "settingsmanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDebug>

SettingsManager& SettingsManager::instance() {
    static SettingsManager instance;
    return instance;
}

SettingsManager::SettingsManager() {
    ensureSettingsDirectory();
    loadSettings();
}

void SettingsManager::ensureSettingsDirectory() {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir dir(configPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
}

QString SettingsManager::getSettingsPath() const {
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(configPath).filePath(m_settingsFile);
}

void SettingsManager::saveSettings() {
    QJsonObject root;
    
    // Сохраняем параметры HoughCircles
    root["houghParams"] = houghParamsToJson(m_houghParams);
    
    // Сохраняем размер превью
    root["previewSize"] = m_previewSize;
    
    // Сохраняем коэффициент μm/pixel
    root["nmPerPixel"] = m_nmPerPixel;
    
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