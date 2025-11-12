// markupimagewidget.cpp - Interactive cell visualization widget
#include "markupimagewidget.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QWheelEvent>
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

    // Calculate maximum circle extension beyond image boundaries
    int maxExtension = 0;
    for (const Cell& cell : m_cells) {
        // Check how far circles extend beyond image boundaries
        int leftExt = qMax(0, static_cast<int>(cell.radius - cell.center_x));
        int rightExt = qMax(0, static_cast<int>((cell.center_x + cell.radius) - m_originalPixmap.width()));
        int topExt = qMax(0, static_cast<int>(cell.radius - cell.center_y));
        int bottomExt = qMax(0, static_cast<int>((cell.center_y + cell.radius) - m_originalPixmap.height()));

        maxExtension = qMax(maxExtension, qMax(qMax(leftExt, rightExt), qMax(topExt, bottomExt)));
    }

    // Add padding for circle thickness and text
    maxExtension += 30;

    // Create extended canvas to allow circles to overflow image boundaries
    int canvasWidth = m_originalPixmap.width() + 2 * maxExtension;
    int canvasHeight = m_originalPixmap.height() + 2 * maxExtension;
    QPixmap displayPixmap(canvasWidth, canvasHeight);
    displayPixmap.fill(Qt::transparent);

    QPainter painter(&displayPixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw original image at center of extended canvas
    painter.drawPixmap(maxExtension, maxExtension, m_originalPixmap);

    // Store offset for coordinate transformation
    m_canvasOffset = maxExtension;

    // Draw all cells with offset coordinates
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

        // Draw circle with offset (now can extend beyond original image boundaries)
        QPointF center(cell.center_x + maxExtension, cell.center_y + maxExtension);
        painter.drawEllipse(center, cell.radius, cell.radius);

        // Draw cell number for selected cell
        if (isSelected) {
            painter.setPen(QPen(QColor(255, 255, 0), 1));
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            painter.drawText(center.x() - 20, center.y() - cell.radius - 10,
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
    // Account for canvas offset (extended canvas for border circles)
    for (int i = 0; i < m_cells.size(); ++i) {
        const Cell& cell = m_cells[i];

        // Calculate distance from click to cell center (with canvas offset)
        double dx = pos.x() - (cell.center_x + m_canvasOffset);
        double dy = pos.y() - (cell.center_y + m_canvasOffset);
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
    , m_zoomFactor(1.0)
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
    m_zoomFactor = 1.0;
}

void MarkupImageWidget::zoomIn()
{
    m_zoomFactor *= 1.25;
    if (m_zoomFactor > 5.0) {
        m_zoomFactor = 5.0;  // Максимальный zoom 500%
    }
    updateZoom();
    emit zoomChanged(m_zoomFactor);
}

void MarkupImageWidget::zoomOut()
{
    m_zoomFactor /= 1.25;
    if (m_zoomFactor < 0.1) {
        m_zoomFactor = 0.1;  // Минимальный zoom 10%
    }
    updateZoom();
    emit zoomChanged(m_zoomFactor);
}

void MarkupImageWidget::resetZoom()
{
    m_zoomFactor = 1.0;
    updateZoom();
    emit zoomChanged(m_zoomFactor);
}

void MarkupImageWidget::fitToWindow()
{
    if (m_currentPixmap.isNull() || !m_scrollArea) {
        return;
    }

    // Вычисляем коэффициент для подгонки под размер окна
    QSize availableSize = m_scrollArea->viewport()->size();
    QSize pixmapSize = m_currentPixmap.size();

    double scaleX = static_cast<double>(availableSize.width()) / pixmapSize.width();
    double scaleY = static_cast<double>(availableSize.height()) / pixmapSize.height();

    m_zoomFactor = qMin(scaleX, scaleY);
    if (m_zoomFactor > 1.0) {
        m_zoomFactor = 1.0;  // Не увеличиваем больше оригинального размера
    }

    updateZoom();
    emit zoomChanged(m_zoomFactor);
}

void MarkupImageWidget::updateZoom()
{
    if (m_currentPixmap.isNull()) {
        return;
    }

    // Масштабируем оригинальное изображение
    QSize newSize = m_currentPixmap.size() * m_zoomFactor;
    QPixmap scaledPixmap = m_currentPixmap.scaled(newSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    m_imageLabel->setOriginalImage(scaledPixmap);

    // Обновляем ячейки с учётом масштаба
    QVector<Cell> scaledCells;
    for (const Cell& cell : m_cells) {
        Cell scaledCell = cell;
        scaledCell.center_x *= m_zoomFactor;
        scaledCell.center_y *= m_zoomFactor;
        scaledCell.radius *= m_zoomFactor;
        scaledCells.append(scaledCell);
    }

    m_imageLabel->setCells(scaledCells);
    LOG_DEBUG(QString("Zoom updated to %1%").arg(m_zoomFactor * 100, 0, 'f', 0));
}

void MarkupImageWidget::wheelEvent(QWheelEvent* event)
{
    // Ctrl + колесико мыши для zoom
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}
