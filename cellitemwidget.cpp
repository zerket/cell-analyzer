// cellitemwidget.cpp
#include "cellitemwidget.h"
#include "utils.h"
#include <QVBoxLayout>

CellItemWidget::CellItemWidget(const Cell& cell, QWidget* parent)
    : QWidget(parent), m_cell(cell)
{
    QImage img = matToQImage(cell.image);
    imageLabel = new QLabel(this);
    imageLabel->setPixmap(QPixmap::fromImage(img).scaled(150,150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLabel->setFixedSize(150,150);

    diameterPxLabel = new QLabel(QString("Диаметр (px): %1").arg(cell.diameterPx), this);

    diameterNmEdit = new QLineEdit(this);
    diameterNmEdit->setPlaceholderText("Диаметр (нм)");
    connect(diameterNmEdit, &QLineEdit::textChanged, this, &CellItemWidget::diameterNmChanged);

    removeButton = new QPushButton("Удалить", this);
    connect(removeButton, &QPushButton::clicked, this, [this]() {
        emit removeRequested(this);
    });

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addWidget(imageLabel);
    layout->addWidget(diameterPxLabel);
    layout->addWidget(diameterNmEdit);
    layout->addWidget(removeButton);

    setLayout(layout);
}

QString CellItemWidget::diameterNmText() const {
    return diameterNmEdit->text();
}

int CellItemWidget::diameterPx() const {
    return m_cell.diameterPx;
}

void CellItemWidget::setDiameterNm(double nm) {
    diameterNmEdit->setText(QString::number(nm, 'f', 2));
}

QImage CellItemWidget::getImage() const {
    // Предполагается, что m_cell.image — cv::Mat, конвертируем в QImage
    return matToQImage(m_cell.image);
}
