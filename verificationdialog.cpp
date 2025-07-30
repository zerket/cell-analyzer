// VerificationDialog.cpp
#include "verificationdialog.h"


VerificationDialog::VerificationDialog(const QVector<QPair<QImage, CellData>>& cells, QWidget* parent)
    : QDialog(parent), m_layout(new QGridLayout(this))
{
    int row = 0, col = 0;
    for (const auto& [img, data] : cells) {
        QCheckBox* cb = new QCheckBox(this);
        cb->setChecked(true);

        QLabel* label = new QLabel;
        label->setPixmap(QPixmap::fromImage(img.scaled(150, 150, Qt::KeepAspectRatio)));

        QVBoxLayout* itemLayout = new QVBoxLayout;
        itemLayout->addWidget(label);
        itemLayout->addWidget(cb);

        m_layout->addLayout(itemLayout, row, col);
        m_checkboxes.append(cb);

        if (++col >= 4) {  // 4 колонки
            col = 0;
            row++;
        }
    }
}

QVector<bool> VerificationDialog::getSelection() const {
    QVector<bool> result;
    for (const auto cb : m_checkboxes) {
        result.append(cb->isChecked());
    }
    return result;
}
