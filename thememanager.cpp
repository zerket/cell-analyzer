#include "thememanager.h"
#include "settingsmanager.h"
#include <QApplication>

ThemeManager& ThemeManager::instance() {
    static ThemeManager instance;
    return instance;
}

ThemeManager::ThemeManager(QObject* parent) : QObject(parent), m_currentTheme(Theme::Light) {
    loadThemeFromSettings();
}

void ThemeManager::setTheme(Theme theme) {
    if (m_currentTheme != theme) {
        m_currentTheme = theme;
        applyTheme(theme);
        saveThemeToSettings();
        emit themeChanged(theme);
    }
}

void ThemeManager::toggleTheme() {
    Theme newTheme = (m_currentTheme == Theme::Light) ? Theme::Dark : Theme::Light;
    setTheme(newTheme);
}

void ThemeManager::loadThemeFromSettings() {
    SettingsManager& settings = SettingsManager::instance();
    QString themeStr = settings.getValue("ui/theme", "light").toString();
    Theme theme = (themeStr == "dark") ? Theme::Dark : Theme::Light;
    setTheme(theme);
}

void ThemeManager::saveThemeToSettings() {
    SettingsManager& settings = SettingsManager::instance();
    QString themeStr = (m_currentTheme == Theme::Dark) ? "dark" : "light";
    settings.setValue("ui/theme", themeStr);
}

void ThemeManager::applyTheme(Theme theme) {
    QString styleSheet;
    if (theme == Theme::Dark) {
        styleSheet = getDarkStyleSheet();
    } else {
        styleSheet = getLightStyleSheet();
    }
    qApp->setStyleSheet(styleSheet);
}

QString ThemeManager::getLightStyleSheet() const {
    return QString(
        "QMainWindow { "
        "    background-color: #f5f5f5; "
        "    color: #333333; "
        "} "
        "QWidget { "
        "    background-color: #ffffff; "
        "    color: #333333; "
        "} "
        "QScrollArea { "
        "    background-color: #ffffff; "
        "    border: 1px solid #cccccc; "
        "} "
        "QLabel { "
        "    color: #333333; "
        "} "
        "QProgressBar { "
        "    background-color: #e0e0e0; "
        "    border: 1px solid #cccccc; "
        "    border-radius: 5px; "
        "    text-align: center; "
        "} "
        "QProgressBar::chunk { "
        "    background-color: #2196F3; "
        "    border-radius: 5px; "
        "} "
        "QSlider::groove:horizontal { "
        "    background-color: #e0e0e0; "
        "    height: 8px; "
        "    border-radius: 4px; "
        "} "
        "QSlider::handle:horizontal { "
        "    background-color: #2196F3; "
        "    border: 1px solid #1976D2; "
        "    width: 18px; "
        "    margin: -5px 0; "
        "    border-radius: 9px; "
        "} "
        + getButtonStyles("#2196F3", "#1976D2", "#ffffff") +
        getScrollBarStyles("#f0f0f0", "#c0c0c0")
    );
}

QString ThemeManager::getDarkStyleSheet() const {
    return QString(
        "QMainWindow { "
        "    background-color: #2b2b2b; "
        "    color: #ffffff; "
        "} "
        "QWidget { "
        "    background-color: #3c3c3c; "
        "    color: #ffffff; "
        "} "
        "QScrollArea { "
        "    background-color: #3c3c3c; "
        "    border: 1px solid #555555; "
        "} "
        "QLabel { "
        "    color: #ffffff; "
        "} "
        "QProgressBar { "
        "    background-color: #555555; "
        "    border: 1px solid #666666; "
        "    border-radius: 5px; "
        "    text-align: center; "
        "    color: #ffffff; "
        "} "
        "QProgressBar::chunk { "
        "    background-color: #0d7377; "
        "    border-radius: 5px; "
        "} "
        "QSlider::groove:horizontal { "
        "    background-color: #555555; "
        "    height: 8px; "
        "    border-radius: 4px; "
        "} "
        "QSlider::handle:horizontal { "
        "    background-color: #0d7377; "
        "    border: 1px solid #0a5d61; "
        "    width: 18px; "
        "    margin: -5px 0; "
        "    border-radius: 9px; "
        "} "
        + getButtonStyles("#0d7377", "#0a5d61", "#ffffff") +
        getScrollBarStyles("#555555", "#777777")
    );
}

QString ThemeManager::getButtonStyles(const QString& bgColor, const QString& hoverColor, const QString& textColor) const {
    return QString(
        "QPushButton { "
        "    background-color: %1; "
        "    color: %3; "
        "    border: none; "
        "    border-radius: 10px; "
        "    padding: 8px 16px; "
        "    font-weight: bold; "
        "} "
        "QPushButton:hover { "
        "    background-color: %2; "
        "} "
        "QPushButton:pressed { "
        "    background-color: %2; "
        "    padding: 9px 15px 7px 17px; "
        "} "
        "QPushButton:disabled { "
        "    background-color: #cccccc; "
        "    color: #666666; "
        "} "
    ).arg(bgColor, hoverColor, textColor);
}

QString ThemeManager::getScrollBarStyles(const QString& bgColor, const QString& handleColor) const {
    return QString(
        "QScrollBar:vertical { "
        "    background: %1; "
        "    width: 15px; "
        "    margin: 0px; "
        "} "
        "QScrollBar::handle:vertical { "
        "    background: %2; "
        "    min-height: 20px; "
        "    border-radius: 7px; "
        "} "
        "QScrollBar::handle:vertical:hover { "
        "    background: #999999; "
        "} "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { "
        "    border: none; "
        "    background: none; "
        "} "
        "QScrollBar:horizontal { "
        "    background: %1; "
        "    height: 15px; "
        "    margin: 0px; "
        "} "
        "QScrollBar::handle:horizontal { "
        "    background: %2; "
        "    min-width: 20px; "
        "    border-radius: 7px; "
        "} "
        "QScrollBar::handle:horizontal:hover { "
        "    background: #999999; "
        "} "
        "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { "
        "    border: none; "
        "    background: none; "
        "} "
    ).arg(bgColor, handleColor);
}