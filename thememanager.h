#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QString>
#include <QApplication>

class ThemeManager : public QObject {
    Q_OBJECT

public:
    enum class Theme {
        Light,
        Dark
    };

    static ThemeManager& instance();
    
    void setTheme(Theme theme);
    Theme getCurrentTheme() const { return m_currentTheme; }
    
    void toggleTheme();
    void loadThemeFromSettings();
    void saveThemeToSettings();
    
    QString getLightStyleSheet() const;
    QString getDarkStyleSheet() const;

signals:
    void themeChanged(Theme newTheme);

private:
    ThemeManager(QObject* parent = nullptr);
    ~ThemeManager() = default;
    
    Theme m_currentTheme;
    void applyTheme(Theme theme);
    
    QString getButtonStyles(const QString& bgColor, const QString& hoverColor, const QString& textColor) const;
    QString getScrollBarStyles(const QString& bgColor, const QString& handleColor) const;
};

#endif // THEMEMANAGER_H