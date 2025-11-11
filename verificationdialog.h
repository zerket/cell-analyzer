// VerificationDialog.h
#pragma once
#include "cell.h"
#include <QDialog>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>

class VerificationDialog : public QDialog {
    Q_OBJECT
public:
    VerificationDialog(const QVector<QPair<QImage, Cell>>& cells,
                       QWidget* parent = nullptr);

    QVector<bool> getSelection() const;

private:
    QGridLayout* m_layout;
    QVector<QCheckBox*> m_checkboxes;
};
