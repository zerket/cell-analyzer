// markupimagewidget.cpp - Simplified stub for YOLO version
#include "markupimagewidget.h"
#include <QPixmap>
#include <QImage>
#include "logger.h"

MarkupImageWidget::MarkupImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_imageLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_selectedCellIndex(-1)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);

    m_imageLabel = new QLabel();
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    m_scrollArea->setWidget(m_imageLabel);
    layout->addWidget(m_scrollArea);

    setLayout(layout);
}

MarkupImageWidget::~MarkupImageWidget()
{
}

void MarkupImageWidget::setImage(const QPixmap& pixmap)
{
    m_currentPixmap = pixmap;
    m_imageLabel->setPixmap(pixmap);
    m_imageLabel->adjustSize();
}

void MarkupImageWidget::setImage(const QString& imagePath)
{
    QPixmap pixmap(imagePath);
    if (!pixmap.isNull()) {
        setImage(pixmap);
    } else {
        LOG_WARNING(QString("Failed to load image: %1").arg(imagePath));
    }
}

void MarkupImageWidget::setCells(const QVector<Cell>& cells)
{
    m_cells = cells;
}

void MarkupImageWidget::setSelectedCell(int index)
{
    m_selectedCellIndex = index;
    // TODO: In full implementation, would highlight the selected cell
}

void MarkupImageWidget::clear()
{
    m_currentPixmap = QPixmap();
    m_imageLabel->clear();
    m_cells.clear();
    m_selectedCellIndex = -1;
}
