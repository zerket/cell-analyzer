// markupimagewidget.cpp
#include "markupimagewidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QScrollBar>
#include <QStyle>
#include <QApplication>
#include <cmath>
#include "logger.h"

// ============================================================================
// MarkupImageLabel Implementation
// ============================================================================

MarkupImageLabel::MarkupImageLabel(QWidget* parent)
    : QLabel(parent)
    , m_selectedCellIndex(-1)
    , m_zoomFactor(1.0)
    , m_minZoom(0.1)
    , m_maxZoom(5.0)
    , m_dragging(false)
{
    setAlignment(Qt::AlignCenter);
    setScaledContents(false);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);
}

void MarkupImageLabel::setImage(const QString& imagePath)
{
    if (imagePath.isEmpty()) {
        LOG_WARNING("setImage: empty image path");
        return;
    }

    m_imagePath = imagePath;
    m_originalPixmap = QPixmap(imagePath);

    if (!m_originalPixmap.isNull()) {
        updateDisplayedPixmap();
    } else {
        LOG_ERROR(QString("Failed to load image: %1").arg(imagePath));
        // Set a placeholder
        clear();
    }
}

void MarkupImageLabel::setCells(const QVector<Cell>& cells)
{
    m_cells = cells;
    update(); // Trigger repaint
}

void MarkupImageLabel::setSelectedCell(int cellIndex)
{
    m_selectedCellIndex = cellIndex;
    update(); // Trigger repaint
}

void MarkupImageLabel::clearSelection()
{
    m_selectedCellIndex = -1;
    update();
}

void MarkupImageLabel::setZoomFactor(double factor)
{
    m_zoomFactor = qBound(m_minZoom, factor, m_maxZoom);
    updateDisplayedPixmap();
    emit zoomChanged(m_zoomFactor);
}

void MarkupImageLabel::zoomIn()
{
    setZoomFactor(m_zoomFactor * 1.2);
}

void MarkupImageLabel::zoomOut()
{
    setZoomFactor(m_zoomFactor / 1.2);
}

void MarkupImageLabel::resetZoom()
{
    setZoomFactor(1.0);
}

void MarkupImageLabel::fitToWindow()
{
    if (m_originalPixmap.isNull() || !parentWidget()) return;

    QWidget* scrollArea = parentWidget()->parentWidget();
    if (!scrollArea) return;

    QSize availableSize = scrollArea->size();
    QSize imageSize = m_originalPixmap.size();

    double widthRatio = static_cast<double>(availableSize.width() - 20) / imageSize.width();
    double heightRatio = static_cast<double>(availableSize.height() - 20) / imageSize.height();

    double fitZoom = qMin(widthRatio, heightRatio);
    setZoomFactor(fitZoom);
}

void MarkupImageLabel::wheelEvent(QWheelEvent* event)
{
    if (event->modifiers() & Qt::ControlModifier) {
        if (event->angleDelta().y() > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
        event->accept();
    } else {
        QLabel::wheelEvent(event);
    }
}

void MarkupImageLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int cellIndex = findCellAtPosition(event->pos());
        if (cellIndex >= 0) {
            emit cellClicked(cellIndex);
        } else {
            m_dragging = true;
            m_lastPanPoint = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
    } else if (event->button() == Qt::RightButton) {
        // Обработка правой кнопки мыши для удаления клетки
        int cellIndex = findCellAtPosition(event->pos());
        if (cellIndex >= 0) {
            emit cellRightClicked(cellIndex);
        }
    }
    QLabel::mousePressEvent(event);
}

void MarkupImageLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_panOffset += delta;
        m_lastPanPoint = event->pos();
        update();
    }
    QLabel::mouseMoveEvent(event);
}

void MarkupImageLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_dragging) {
        m_dragging = false;
        setCursor(Qt::CrossCursor);
    }
    QLabel::mouseReleaseEvent(event);
}

void MarkupImageLabel::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);

    if (!m_scaledPixmap.isNull()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        drawCellsOverlay(painter);
    }
}

void MarkupImageLabel::updateDisplayedPixmap()
{
    if (m_originalPixmap.isNull()) return;

    QSize scaledSize = m_originalPixmap.size() * m_zoomFactor;
    m_scaledPixmap = m_originalPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    setPixmap(m_scaledPixmap);
    update();
}

void MarkupImageLabel::drawCellsOverlay(QPainter& painter)
{
    if (m_cells.isEmpty()) return;

    for (int i = 0; i < m_cells.size(); ++i) {
        const Cell& cell = m_cells[i];

        // Scale coordinates according to zoom
        int centerX = static_cast<int>(cell.circle[0] * m_zoomFactor);
        int centerY = static_cast<int>(cell.circle[1] * m_zoomFactor);
        int radius = static_cast<int>(cell.circle[2] * m_zoomFactor);

        // Calculate position on widget (accounting for label alignment)
        QPoint labelOffset((width() - m_scaledPixmap.width()) / 2, (height() - m_scaledPixmap.height()) / 2);
        QPoint center = labelOffset + QPoint(centerX, centerY);

        // Draw cell rectangle
        bool isSelected = (i == m_selectedCellIndex);
        QColor color = isSelected ? QColor(255, 255, 0) : QColor(255, 0, 0); // Yellow if selected, red otherwise
        int penWidth = isSelected ? 3 : 2;

        painter.setPen(QPen(color, penWidth));
        painter.setBrush(Qt::NoBrush);

        QRect rect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);
        painter.drawRect(rect);

        // Draw cell number
        if (isSelected) {
            painter.setPen(QPen(Qt::white, 1));
            painter.setBrush(QColor(255, 193, 7, 200));
            QRect textRect(center.x() - radius, center.y() - radius - 20, 30, 18);
            painter.drawRect(textRect);

            painter.setPen(Qt::black);
            QFont font = painter.font();
            font.setBold(true);
            painter.setFont(font);
            painter.drawText(textRect, Qt::AlignCenter, QString("#%1").arg(i + 1));
        }
    }
}

int MarkupImageLabel::findCellAtPosition(const QPoint& pos)
{
    if (m_cells.isEmpty() || m_scaledPixmap.isNull()) return -1;

    QPoint labelOffset((width() - m_scaledPixmap.width()) / 2, (height() - m_scaledPixmap.height()) / 2);

    for (int i = 0; i < m_cells.size(); ++i) {
        const Cell& cell = m_cells[i];

        int centerX = static_cast<int>(cell.circle[0] * m_zoomFactor);
        int centerY = static_cast<int>(cell.circle[1] * m_zoomFactor);
        int radius = static_cast<int>(cell.circle[2] * m_zoomFactor);

        QPoint center = labelOffset + QPoint(centerX, centerY);
        QRect cellRect(center.x() - radius, center.y() - radius, radius * 2, radius * 2);

        if (cellRect.contains(pos)) {
            return i;
        }
    }

    return -1;
}

QPoint MarkupImageLabel::mapToOriginalImage(const QPoint& widgetPos) const
{
    QPoint labelOffset((width() - m_scaledPixmap.width()) / 2, (height() - m_scaledPixmap.height()) / 2);
    QPoint imagePos = widgetPos - labelOffset;

    int x = static_cast<int>(imagePos.x() / m_zoomFactor);
    int y = static_cast<int>(imagePos.y() / m_zoomFactor);

    return QPoint(x, y);
}

// ============================================================================
// MarkupImageWidget Implementation
// ============================================================================

MarkupImageWidget::MarkupImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_updatingControls(false)
{
    setupUI();
}

void MarkupImageWidget::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5);

    // Scroll area with image label
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setAlignment(Qt::AlignCenter);

    m_imageLabel = new MarkupImageLabel(this);
    m_scrollArea->setWidget(m_imageLabel);

    connect(m_imageLabel, &MarkupImageLabel::zoomChanged, this, &MarkupImageWidget::onImageZoomChanged);
    connect(m_imageLabel, &MarkupImageLabel::cellClicked, this, &MarkupImageWidget::cellClicked);
    connect(m_imageLabel, &MarkupImageLabel::cellRightClicked, this, &MarkupImageWidget::cellRightClicked);

    mainLayout->addWidget(m_scrollArea);

    // Toolbar
    setupToolbar();

    setLayout(mainLayout);
}

void MarkupImageWidget::setupToolbar()
{
    QHBoxLayout* toolbarLayout = new QHBoxLayout();
    toolbarLayout->setContentsMargins(5, 0, 5, 5);

    // Zoom buttons
    m_zoomInButton = new QPushButton("🔍+");
    m_zoomInButton->setFixedSize(35, 30);
    m_zoomInButton->setToolTip("Zoom In (Ctrl+Wheel Up)");
    connect(m_zoomInButton, &QPushButton::clicked, this, &MarkupImageWidget::zoomIn);

    m_zoomOutButton = new QPushButton("🔍-");
    m_zoomOutButton->setFixedSize(35, 30);
    m_zoomOutButton->setToolTip("Zoom Out (Ctrl+Wheel Down)");
    connect(m_zoomOutButton, &QPushButton::clicked, this, &MarkupImageWidget::zoomOut);

    m_resetZoomButton = new QPushButton("100%");
    m_resetZoomButton->setFixedSize(45, 30);
    m_resetZoomButton->setToolTip("Reset Zoom");
    connect(m_resetZoomButton, &QPushButton::clicked, this, &MarkupImageWidget::resetZoom);

    m_fitWindowButton = new QPushButton("Fit");
    m_fitWindowButton->setFixedSize(40, 30);
    m_fitWindowButton->setToolTip("Fit to Window");
    connect(m_fitWindowButton, &QPushButton::clicked, this, &MarkupImageWidget::fitToWindow);

    // Zoom slider
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setMinimum(10);
    m_zoomSlider->setMaximum(500);
    m_zoomSlider->setValue(100);
    m_zoomSlider->setFixedWidth(150);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &MarkupImageWidget::onZoomSliderChanged);

    // Zoom spinbox
    m_zoomSpin = new QSpinBox();
    m_zoomSpin->setMinimum(10);
    m_zoomSpin->setMaximum(500);
    m_zoomSpin->setValue(100);
    m_zoomSpin->setSuffix("%");
    m_zoomSpin->setFixedWidth(70);
    connect(m_zoomSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MarkupImageWidget::onZoomSpinChanged);

    toolbarLayout->addWidget(m_zoomInButton);
    toolbarLayout->addWidget(m_zoomOutButton);
    toolbarLayout->addWidget(m_resetZoomButton);
    toolbarLayout->addWidget(m_fitWindowButton);
    toolbarLayout->addSpacing(10);
    toolbarLayout->addWidget(new QLabel("Zoom:"));
    toolbarLayout->addWidget(m_zoomSlider);
    toolbarLayout->addWidget(m_zoomSpin);
    toolbarLayout->addStretch();

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (mainLayout) {
        mainLayout->addLayout(toolbarLayout);
    }
}

void MarkupImageWidget::setImage(const QString& imagePath)
{
    m_imageLabel->setImage(imagePath);
}

void MarkupImageWidget::setCells(const QVector<Cell>& cells)
{
    m_imageLabel->setCells(cells);
}

void MarkupImageWidget::setSelectedCell(int cellIndex)
{
    m_imageLabel->setSelectedCell(cellIndex);
}

void MarkupImageWidget::clearSelection()
{
    m_imageLabel->clearSelection();
}

double MarkupImageWidget::getZoomFactor() const
{
    return m_imageLabel->getZoomFactor();
}

void MarkupImageWidget::setZoomFactor(double factor)
{
    m_imageLabel->setZoomFactor(factor);
}

void MarkupImageWidget::zoomIn()
{
    m_imageLabel->zoomIn();
}

void MarkupImageWidget::zoomOut()
{
    m_imageLabel->zoomOut();
}

void MarkupImageWidget::resetZoom()
{
    m_imageLabel->resetZoom();
}

void MarkupImageWidget::fitToWindow()
{
    m_imageLabel->fitToWindow();
}

void MarkupImageWidget::onZoomSliderChanged(int value)
{
    if (m_updatingControls) return;
    double factor = value / 100.0;
    m_imageLabel->setZoomFactor(factor);
}

void MarkupImageWidget::onZoomSpinChanged(int value)
{
    if (m_updatingControls) return;
    double factor = value / 100.0;
    m_imageLabel->setZoomFactor(factor);
}

void MarkupImageWidget::onImageZoomChanged(double factor)
{
    updateZoomControls(factor);
    emit zoomChanged(factor);
}

void MarkupImageWidget::updateZoomControls(double factor)
{
    m_updatingControls = true;

    int percent = static_cast<int>(factor * 100);
    m_zoomSlider->setValue(percent);
    m_zoomSpin->setValue(percent);

    m_updatingControls = false;
}
