// VerificationDialog.h
#pragma once
#include "imageprocessor.h"
#include <QDialog>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

class VerificationDialog : public QDialog {
    Q_OBJECT
public:
    VerificationDialog(const QVector<QPair<QImage,
                             CellData>>& cells,
                             QWidget* parent = nullptr);

    QVector<bool> getSelection() const;

private:
    QGridLayout* m_layout;
    QVector<QCheckBox*> m_checkboxes;
};
