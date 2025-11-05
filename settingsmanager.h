#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QString>
#include <QJsonObject>
#include <QVariant>
#include "imageprocessor.h"

class SettingsManager {
public:
    static SettingsManager& instance();
    
    void saveSettings();
    void loadSettings();
    
    // Параметры HoughCircles
    ImageProcessor::HoughParams getHoughParams() const { return m_houghParams; }
    void setHoughParams(const ImageProcessor::HoughParams& params);
    
    // Размер превью
    int getPreviewSize() const { return m_previewSize; }
    void setPreviewSize(int size);
    
    // Коэффициент μm/pixel
    double getNmPerPixel() const { return m_nmPerPixel; }
    void setNmPerPixel(double coefficient);

    // Пороги для статистики (мкм)
    double getStatisticsMinThreshold() const { return m_statisticsMinThreshold; }
    void setStatisticsMinThreshold(double threshold);
    double getStatisticsMaxThreshold() const { return m_statisticsMaxThreshold; }
    void setStatisticsMaxThreshold(double threshold);

    // Путь к файлу настроек
    QString getSettingsPath() const;
    
    // Общие методы для настроек
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

    // Методы для работы с пресетами
    QJsonObject getPresets() const;
    void setPresets(const QJsonObject& presets);
    QString getLastSelectedPreset() const;
    void setLastSelectedPreset(const QString& presetName);

private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;
    
    void ensureSettingsDirectory();
    QJsonObject houghParamsToJson(const ImageProcessor::HoughParams& params) const;
    ImageProcessor::HoughParams jsonToHoughParams(const QJsonObject& json) const;
    
    ImageProcessor::HoughParams m_houghParams;
    int m_previewSize = 150;
    double m_nmPerPixel = 0.0;
    double m_statisticsMinThreshold = 50.0;  // По умолчанию 50 мкм
    double m_statisticsMaxThreshold = 100.0; // По умолчанию 100 мкм
    QString m_settingsFile = "settings.json";
    mutable QJsonObject m_settings;
};

#endif // SETTINGSMANAGER_H