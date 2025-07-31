// cellitemwidget.cpp
#include "cellitemwidget.h"
#include "utils.h"
#include <QVBoxLayout>
#include "logger.h"

CellItemWidget::CellItemWidget(const Cell& cell, QWidget* parent)
    : QWidget(parent), m_cell(cell)
{
    LOG_DEBUG("CellItemWidget constructor called");
    LOG_DEBUG(QString("Cell info: diameterPx=%1, diameterNm=%2, image size=%3x%4")
        .arg(cell.diameterPx)
        .arg(cell.diameterNm)
        .arg(cell.image.cols)
        .arg(cell.image.rows));
    
    try {
        LOG_DEBUG("Converting cv::Mat to QImage");
        QImage img = matToQImage(cell.image);
        LOG_DEBUG(QString("QImage created: %1x%2").arg(img.width()).arg(img.height()));
        
        imageLabel = new QLabel(this);
        imageLabel->setPixmap(QPixmap::fromImage(img).scaled(150,150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        imageLabel->setFixedSize(150,150);
        LOG_DEBUG("Image label created and configured");

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
    
    LOG_DEBUG("CellItemWidget layout completed");
    } catch (const std::exception& e) {
        LOG_ERROR(QString("Exception in CellItemWidget constructor: %1").arg(e.what()));
        throw;
    } catch (...) {
        LOG_ERROR("Unknown exception in CellItemWidget constructor");
        throw;
    }
    
    LOG_DEBUG("CellItemWidget constructor completed successfully");
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
