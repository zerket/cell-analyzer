#include "settingsmanager.h"
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
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

    // Сохраняем пресеты как массив
    QJsonArray presetsArray;
    for (const auto& preset : m_presets) {
        presetsArray.append(yoloParamsToJson(preset));
    }
    root["yoloParams"] = presetsArray;

    // Сохраняем размер превью
    root["previewSize"] = m_previewSize;

    // Сохраняем пороги статистики
    root["statisticsMinThreshold"] = m_statisticsMinThreshold;
    root["statisticsMaxThreshold"] = m_statisticsMaxThreshold;

    // Сохраняем тему
    root["Theme"] = m_theme;

    // Сохраняем последний выбранный пресет
    if (m_settings.contains("lastSelectedPreset")) {
        root["lastSelectedPreset"] = m_settings["lastSelectedPreset"];
    }

    // Записываем в файл
    QJsonDocument doc(root);
    QFile file(getSettingsPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson(QJsonDocument::Indented));
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
        // Создаем дефолтный пресет
        ImageProcessor::YoloParams defaultPreset;
        defaultPreset.modelPath = "ml-data/models/yolov8s_cells_v1.0.pt";
        defaultPreset.confThreshold = 0.25;
        defaultPreset.iouThreshold = 0.7;
        defaultPreset.minCellArea = 500;
        defaultPreset.device = "0";
        m_presets.append(defaultPreset);
        saveSettings(); // Создаем файл с настройками по умолчанию
        return;
    }

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject root = doc.object();

            // Загружаем пресеты из массива (поддерживаем старые и новые форматы)
            if (root.contains("yoloParams") && root["yoloParams"].isArray()) {
                QJsonArray presetsArray = root["yoloParams"].toArray();
                m_presets.clear();
                for (const QJsonValue& value : presetsArray) {
                    if (value.isObject()) {
                        m_presets.append(jsonToYoloParams(value.toObject()));
                    }
                }
            } else if (root.contains("houghParams") && root["houghParams"].isArray()) {
                // Старый формат - создаем default preset
                m_presets.clear();
                ImageProcessor::YoloParams defaultPreset;
                m_presets.append(defaultPreset);
            }

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

            // Загружаем тему
            if (root.contains("Theme")) {
                m_theme = root["Theme"].toString("Dark");
            }

            // Сохраняем весь объект для общих настроек
            m_settings = root;

            qDebug() << "Settings loaded from:" << getSettingsPath();
        }
    } else {
        qDebug() << "Failed to load settings from:" << getSettingsPath();
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

void SettingsManager::setTheme(const QString& theme) {
    m_theme = theme;
    saveSettings();
}

QJsonObject SettingsManager::houghParamsToJson(const ImageProcessor::HoughParams& params) const {
    QJsonObject json;
    json["name"] = params.name;
    json["dp"] = params.dp;
    json["minDist"] = params.minDist;
    json["param1"] = params.param1;
    json["param2"] = params.param2;
    json["minRadius"] = params.minRadius;
    json["maxRadius"] = params.maxRadius;
    json["umPerPixel"] = params.umPerPixel;
    return json;
}

ImageProcessor::HoughParams SettingsManager::jsonToHoughParams(const QJsonObject& json) const {
    ImageProcessor::HoughParams params;
    params.name = json["name"].toString("default");
    params.dp = json["dp"].toDouble(1.0);
    params.minDist = json["minDist"].toDouble(30.0);
    params.param1 = json["param1"].toDouble(90.0);
    params.param2 = json["param2"].toDouble(50.0);
    params.minRadius = json["minRadius"].toInt(30);
    params.maxRadius = json["maxRadius"].toInt(150);
    params.umPerPixel = json["umPerPixel"].toDouble(0.0);
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

// Методы для работы с пресетами
QVector<ImageProcessor::HoughParams> SettingsManager::getAllPresets() const {
    return m_presets;
}

void SettingsManager::setAllPresets(const QVector<ImageProcessor::HoughParams>& presets) {
    m_presets = presets;
    saveSettings();
}

ImageProcessor::HoughParams SettingsManager::getPresetByName(const QString& name) const {
    for (const auto& preset : m_presets) {
        if (preset.name == name) {
            return preset;
        }
    }
    // Возвращаем первый пресет, если не найден
    if (!m_presets.isEmpty()) {
        return m_presets.first();
    }
    // Возвращаем дефолтный
    return ImageProcessor::HoughParams();
}

void SettingsManager::savePreset(const ImageProcessor::HoughParams& preset) {
    // Ищем существующий пресет с таким же именем
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == preset.name) {
            m_presets[i] = preset;
            saveSettings();
            return;
        }
    }
    // Если не найден, добавляем новый
    m_presets.append(preset);
    saveSettings();
}

void SettingsManager::deletePreset(const QString& name) {
    for (int i = 0; i < m_presets.size(); ++i) {
        if (m_presets[i].name == name) {
            m_presets.removeAt(i);
            saveSettings();
            return;
        }
    }
}

QString SettingsManager::getLastSelectedPreset() const {
    return getValue("lastSelectedPreset", "default").toString();
}

void SettingsManager::setLastSelectedPreset(const QString& presetName) {
    setValue("lastSelectedPreset", presetName);
}
