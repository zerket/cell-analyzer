#include "cellitem.h"
#include "utils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIntValidator>
#include <QPixmap>

CellItem::CellItem(const Cell& cell, QWidget* parent)
    : QWidget(parent), m_cell(cell) {

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    imageLabel = new QLabel(this);
    updateImage();
    mainLayout->addWidget(imageLabel, 0, Qt::AlignCenter);

    diameterPxLabel = new QLabel(QString("Диаметр (px): %1").arg(m_cell.diameterPx), this);
    mainLayout->addWidget(diameterPxLabel);

    QHBoxLayout* nmLayout = new QHBoxLayout;
    nmLayout->addWidget(new QLabel("Диаметр (мкм):", this));

    diameterNmEdit = new QLineEdit(this);
    diameterNmEdit->setPlaceholderText("Введите диаметр в мкм");
    diameterNmEdit->setValidator(new QDoubleValidator(0, 1e6, 2, this));
    //diameterNmEdit->setEnabled(true);
    nmLayout->addWidget(diameterNmEdit);

    mainLayout->addLayout(nmLayout);

    excludeButton = new QPushButton("Исключить", this);
    //excludeButton->setEnabled(true);
    mainLayout->addWidget(excludeButton);

    connect(diameterNmEdit, &QLineEdit::textChanged, this, &CellItem::onDiameterNmEdited);
    connect(excludeButton, &QPushButton::clicked, this, &CellItem::onExcludeClicked);
}

void CellItem::updateImage() {
    QImage img = matToQImage(m_cell.image);
    if (img.isNull()) return;

    QPixmap pixmap = QPixmap::fromImage(img);

    int maxSize = 180;
    QSize scaledSize = pixmap.size();
    scaledSize.scale(maxSize, maxSize, Qt::KeepAspectRatio);

    imageLabel->setPixmap(pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imageLabel->setFixedSize(scaledSize);
}

int CellItem::diameterPx() const {
    return m_cell.diameterPx;
}

double CellItem::diameterNm() const {
    return diameterNmEdit->text().toDouble();
}

void CellItem::setDiameterNm(double nm) {
    diameterNmEdit->setText(QString::number(nm, 'f', 2));
}

bool CellItem::isExcluded() const {
    return excluded;
}

void CellItem::onDiameterNmEdited(const QString& text) {
    emit diameterNmChanged();
}

void CellItem::onExcludeClicked() {
    excluded = true;
    this->hide();
    emit excludedChanged();
}

QString CellItem::diameterNmText() const {
    return diameterNmEdit->text();
}
