// markupimagewidget.cpp - Interactive cell visualization widget
#include "markupimagewidget.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <cmath>
#include "logger.h"

// ============================================================================
// InteractiveImageLabel Implementation
// ============================================================================

InteractiveImageLabel::InteractiveImageLabel(QWidget* parent)
    : QLabel(parent)
    , m_selectedCellIndex(-1)
{
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void InteractiveImageLabel::setCells(const QVector<Cell>& cells)
{
    m_cells = cells;
    updateDisplay();
}

void InteractiveImageLabel::setSelectedCell(int index)
{
    m_selectedCellIndex = index;
    updateDisplay();
}

void InteractiveImageLabel::setOriginalImage(const QPixmap& pixmap)
{
    m_originalPixmap = pixmap;
    updateDisplay();
}

void InteractiveImageLabel::updateDisplay()
{
    if (m_originalPixmap.isNull()) {
        return;
    }

    // Create a copy to draw on
    QPixmap displayPixmap = m_originalPixmap.copy();
    QPainter painter(&displayPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw all cells
    for (int i = 0; i < m_cells.size(); ++i) {
        const Cell& cell = m_cells[i];

        // Determine color based on selection
        bool isSelected = (i == m_selectedCellIndex);

        if (isSelected) {
            // Selected cell: thick red circle
            painter.setPen(QPen(QColor(255, 0, 0), 3));
            painter.setBrush(Qt::NoBrush);
        } else {
            // Normal cell: green circle
            painter.setPen(QPen(QColor(0, 255, 0), 2));
            painter.setBrush(Qt::NoBrush);
        }

        // Draw circle
        painter.drawEllipse(QPointF(cell.center_x, cell.center_y), cell.radius, cell.radius);

        // Draw cell number for selected cell
        if (isSelected) {
            painter.setPen(QPen(QColor(255, 255, 0), 1));
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            painter.drawText(cell.center_x - 20, cell.center_y - cell.radius - 10,
                           QString::number(i + 1));
        }
    }

    painter.end();

    setPixmap(displayPixmap);
    adjustSize();
}

void InteractiveImageLabel::mousePressEvent(QMouseEvent* event)
{
    int cellIndex = findCellAtPosition(event->pos());

    if (cellIndex >= 0) {
        if (event->button() == Qt::LeftButton) {
            emit cellClicked(cellIndex);
        } else if (event->button() == Qt::RightButton) {
            emit cellRightClicked(cellIndex);
        }
    }

    QLabel::mousePressEvent(event);
}

void InteractiveImageLabel::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);
}

int InteractiveImageLabel::findCellAtPosition(const QPoint& pos)
{
    // Find the cell closest to the click position
    for (int i = 0; i < m_cells.size(); ++i) {
        const Cell& cell = m_cells[i];

        // Calculate distance from click to cell center
        double dx = pos.x() - cell.center_x;
        double dy = pos.y() - cell.center_y;
        double distance = std::sqrt(dx * dx + dy * dy);

        // Check if click is within cell radius
        if (distance <= cell.radius) {
            return i;
        }
    }

    return -1;  // No cell found
}

// ============================================================================
// MarkupImageWidget Implementation
// ============================================================================

MarkupImageWidget::MarkupImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_imageLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_selectedCellIndex(-1)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(false);  // Allow scrolling
    m_scrollArea->setAlignment(Qt::AlignCenter);

    m_imageLabel = new InteractiveImageLabel();
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    // Connect signals from InteractiveImageLabel
    connect(m_imageLabel, &InteractiveImageLabel::cellClicked,
            this, &MarkupImageWidget::cellClicked);
    connect(m_imageLabel, &InteractiveImageLabel::cellRightClicked,
            this, &MarkupImageWidget::cellRightClicked);

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
    m_imageLabel->setOriginalImage(pixmap);
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
    m_imageLabel->setCells(cells);
}

void MarkupImageWidget::setSelectedCell(int index)
{
    m_selectedCellIndex = index;
    m_imageLabel->setSelectedCell(index);
}

void MarkupImageWidget::clear()
{
    m_currentPixmap = QPixmap();
    m_imageLabel->setOriginalImage(QPixmap());
    m_imageLabel->clear();
    m_cells.clear();
    m_selectedCellIndex = -1;
}
