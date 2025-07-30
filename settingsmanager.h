#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QString>
#include <QJsonObject>
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
    
    // Путь к файлу настроек
    QString getSettingsPath() const;

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
    QString m_settingsFile = "settings.json";
};

#endif // SETTINGSMANAGER_H