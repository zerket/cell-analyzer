#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>

class SettingsManager {
public:
    static SettingsManager& instance();

    void saveSettings();
    void loadSettings();

    // Размер превью
    int getPreviewSize() const { return m_previewSize; }
    void setPreviewSize(int size);

    // Пороги для статистики (мкм)
    double getStatisticsMinThreshold() const { return m_statisticsMinThreshold; }
    void setStatisticsMinThreshold(double threshold);
    double getStatisticsMaxThreshold() const { return m_statisticsMaxThreshold; }
    void setStatisticsMaxThreshold(double threshold);

    // Коэффициент пересчета (мкм/пиксель)
    double getCoefficient() const { return m_coefficient; }
    void setCoefficient(double coefficient);

    // Тема
    QString getTheme() const { return m_theme; }
    void setTheme(const QString& theme);

    // Путь к файлу настроек
    QString getSettingsPath() const;

    // Общие методы для настроек
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

private:
    SettingsManager();
    ~SettingsManager() = default;
    SettingsManager(const SettingsManager&) = delete;
    SettingsManager& operator=(const SettingsManager&) = delete;

    void ensureSettingsDirectory();

    int m_previewSize = 150;
    double m_statisticsMinThreshold = 50.0;  // По умолчанию 50 мкм
    double m_statisticsMaxThreshold = 100.0; // По умолчанию 100 мкм
    double m_coefficient = 0.0;              // Коэффициент мкм/пиксель
    QString m_theme = "Dark";
    QString m_settingsFile = "settings.json";
    mutable QJsonObject m_settings;
};

#endif // SETTINGSMANAGER_H