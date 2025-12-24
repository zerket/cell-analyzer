#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>
#include <QSlider>
#include <QLabel>

class SettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    ~SettingsDialog() = default;

private slots:
    void onSaveClicked();
    void onCancelClicked();
    void onChooseHighlightColorClicked();
    void onChooseSelectionColorClicked();
    void onHighlightColorTextChanged(const QString& text);
    void onSelectionColorTextChanged(const QString& text);
    void onIgnoreBorderCellsToggled(bool checked);
    void onVisibilityThresholdChanged(int value);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void updateHighlightColorPreview();
    void updateSelectionColorPreview();
    bool validateInput();

private:
    // Coefficient
    QLineEdit* m_coefficientEdit;

    // Statistics thresholds
    QLineEdit* m_minThresholdEdit;
    QLineEdit* m_maxThresholdEdit;

    // Cell highlight color (all cells)
    QLineEdit* m_highlightColorEdit;
    QPushButton* m_highlightColorPickerButton;
    QWidget* m_highlightColorPreview;

    // Cell selection color (selected cell)
    QLineEdit* m_selectionColorEdit;
    QPushButton* m_selectionColorPickerButton;
    QWidget* m_selectionColorPreview;

    // Border cells filter
    QCheckBox* m_ignoreBorderCellsCheckbox;
    QSlider* m_visibilityThresholdSlider;
    QLabel* m_visibilityThresholdValueLabel;

    // Cell diameter filter
    QLineEdit* m_minDiameterEdit;
    QLineEdit* m_maxDiameterEdit;

    // Buttons
    QPushButton* m_saveButton;
    QPushButton* m_cancelButton;

    QColor m_currentHighlightColor;
    QColor m_currentSelectionColor;
};

#endif // SETTINGSDIALOG_H
