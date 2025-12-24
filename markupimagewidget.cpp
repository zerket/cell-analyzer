// markupimagewidget.cpp - Interactive cell visualization widget
#include "markupimagewidget.h"
#include <QPixmap>
#include <QImage>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QHBoxLayout>
#include <QScrollBar>
#include <cmath>
#include <limits>
#include "logger.h"
#include "settingsmanager.h"

// ============================================================================
// InteractiveImageLabel Implementation
// ============================================================================

InteractiveImageLabel::InteractiveImageLabel(QWidget* parent)
    : QLabel(parent)
    , m_selectedCellIndex(-1)
    , m_zoomFactor(1.0)
    , m_minZoom(0.1)
    , m_maxZoom(5.0)
    , m_dragging(false)
    , m_panOffset(0, 0)
{
    setMouseTracking(true);
    setCursor(Qt::OpenHandCursor);
    setAlignment(Qt::AlignCenter);
    setMinimumSize(100, 100);
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

    // Load colors from settings
    SettingsManager& settings = SettingsManager::instance();
    QColor cellHighlightColor(settings.getCellHighlightColor());  // Color for all cells
    QColor cellSelectionColor(settings.getCellSelectionColor());  // Color for selected cell

    // Draw all cells with offset coordinates (scaled by zoom)
    for (int i = 0; i < m_cells.size(); ++i) {
        const Cell& cell = m_cells[i];

        // Determine color based on selection
        bool isSelected = (i == m_selectedCellIndex);

        if (isSelected) {
            // Selected cell: use selection color from settings
            painter.setPen(QPen(cellSelectionColor, 3));
            painter.setBrush(Qt::NoBrush);
        } else {
            // Normal cell: use highlight color from settings
            painter.setPen(QPen(cellHighlightColor, 2));
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

    // Apply zoom scaling
    QSize scaledSize = displayPixmap.size() * m_zoomFactor;
    QPixmap scaledPixmap = displayPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    setPixmap(scaledPixmap);
    resize(scaledPixmap.size());
}

void InteractiveImageLabel::setZoomFactor(double factor)
{
    factor = qBound(m_minZoom, factor, m_maxZoom);

    if (qAbs(m_zoomFactor - factor) > 0.001) {
        m_zoomFactor = factor;
        updateDisplay();
        emit zoomChanged(m_zoomFactor);
    }
}

void InteractiveImageLabel::zoomIn()
{
    setZoomFactor(m_zoomFactor * 1.25);
}

void InteractiveImageLabel::zoomOut()
{
    setZoomFactor(m_zoomFactor / 1.25);
}

void InteractiveImageLabel::resetZoom()
{
    setZoomFactor(1.0);
    m_panOffset = QPoint(0, 0);
    updateDisplay();
}

void InteractiveImageLabel::fitToWindow()
{
    if (m_originalPixmap.isNull() || !parentWidget()) return;

    QSize parentSize = parentWidget()->size();
    QSize imageSize = m_originalPixmap.size();

    double scaleX = double(parentSize.width()) / imageSize.width();
    double scaleY = double(parentSize.height()) / imageSize.height();
    double scale = qMin(scaleX, scaleY) * 0.9; // 90% от размера окна

    setZoomFactor(scale);
    m_panOffset = QPoint(0, 0);
}

void InteractiveImageLabel::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        int cellIndex = findCellAtPosition(event->pos());

        if (cellIndex >= 0) {
            // Clicked on a cell - select it
            emit cellClicked(cellIndex);
            m_dragging = false;
        } else {
            // Clicked outside cells - start panning
            m_dragging = true;
            m_lastPanPoint = event->pos();
            setCursor(Qt::ClosedHandCursor);
        }
    } else if (event->button() == Qt::RightButton) {
        int cellIndex = findCellAtPosition(event->pos());
        if (cellIndex >= 0) {
            emit cellRightClicked(cellIndex);
        }
    }

    QLabel::mousePressEvent(event);
}

void InteractiveImageLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (m_dragging && (event->buttons() & Qt::LeftButton)) {
        // Calculate scroll delta
        QPoint delta = event->pos() - m_lastPanPoint;
        m_lastPanPoint = event->pos();

        // Get parent scroll area and adjust scroll position
        QScrollArea* scrollArea = qobject_cast<QScrollArea*>(parent()->parent());
        if (scrollArea) {
            QScrollBar* hBar = scrollArea->horizontalScrollBar();
            QScrollBar* vBar = scrollArea->verticalScrollBar();

            if (hBar) {
                hBar->setValue(hBar->value() - delta.x());
            }
            if (vBar) {
                vBar->setValue(vBar->value() - delta.y());
            }
        }
    }

    QLabel::mouseMoveEvent(event);
}

void InteractiveImageLabel::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        if (m_dragging) {
            m_dragging = false;
            setCursor(Qt::OpenHandCursor);
        }
    }
    QLabel::mouseReleaseEvent(event);
}

void InteractiveImageLabel::wheelEvent(QWheelEvent* event)
{
    const double scaleFactor = 1.15;

    if (event->angleDelta().y() > 0) {
        setZoomFactor(m_zoomFactor * scaleFactor);
    } else {
        setZoomFactor(m_zoomFactor / scaleFactor);
    }

    event->accept();
}

void InteractiveImageLabel::paintEvent(QPaintEvent* event)
{
    QLabel::paintEvent(event);
}

QPoint InteractiveImageLabel::mapToOriginalImage(const QPoint& widgetPos) const
{
    if (m_originalPixmap.isNull()) return QPoint(-1, -1);

    // Account for zoom and canvas offset
    double invZoom = 1.0 / m_zoomFactor;
    QPoint originalPos(
        static_cast<int>((widgetPos.x() * invZoom) - m_canvasOffset),
        static_cast<int>((widgetPos.y() * invZoom) - m_canvasOffset)
    );

    return originalPos;
}

int InteractiveImageLabel::findCellAtPosition(const QPoint& pos)
{
    // Find the cell closest to the click position
    // Account for canvas offset and zoom

    // Iterate through all cells and find the closest one within its radius
    int closestIndex = -1;
    double closestDistance = std::numeric_limits<double>::max();

    for (int i = 0; i < m_cells.size(); ++i) {
        const Cell& cell = m_cells[i];

        // Calculate distance from click to cell center (with canvas offset and zoom)
        double scaledCenterX = (cell.center_x + m_canvasOffset) * m_zoomFactor;
        double scaledCenterY = (cell.center_y + m_canvasOffset) * m_zoomFactor;
        double scaledRadius = cell.radius * m_zoomFactor;

        double dx = pos.x() - scaledCenterX;
        double dy = pos.y() - scaledCenterY;
        double distance = std::sqrt(dx * dx + dy * dy);

        // Check if click is within cell radius
        if (distance <= scaledRadius && distance < closestDistance) {
            closestDistance = distance;
            closestIndex = i;
        }
    }

    if (closestIndex >= 0) {
        LOG_DEBUG(QString("Cell found at index %1 (distance: %2 px)").arg(closestIndex).arg(closestDistance, 0, 'f', 1));
    }

    return closestIndex;
}

// ============================================================================
// MarkupImageWidget Implementation
// ============================================================================

MarkupImageWidget::MarkupImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_imageLabel(nullptr)
    , m_scrollArea(nullptr)
    , m_selectedCellIndex(-1)
    , m_toolbar(nullptr)
    , m_zoomSlider(nullptr)
    , m_zoomSpin(nullptr)
    , m_updatingControls(false)
{
    setupUI();
}

MarkupImageWidget::~MarkupImageWidget()
{
}

void MarkupImageWidget::setupUI()
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    setupToolbar();
    layout->addWidget(m_toolbar);

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(false);  // Allow scrolling
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; }");

    m_imageLabel = new InteractiveImageLabel();
    m_imageLabel->setScaledContents(false);
    m_imageLabel->setAlignment(Qt::AlignCenter);

    // Connect signals from InteractiveImageLabel
    connect(m_imageLabel, &InteractiveImageLabel::cellClicked,
            this, &MarkupImageWidget::cellClicked);
    connect(m_imageLabel, &InteractiveImageLabel::cellRightClicked,
            this, &MarkupImageWidget::cellRightClicked);
    connect(m_imageLabel, &InteractiveImageLabel::zoomChanged,
            this, &MarkupImageWidget::onImageZoomChanged);

    m_scrollArea->setWidget(m_imageLabel);
    layout->addWidget(m_scrollArea, 1);

    setLayout(layout);
}

void MarkupImageWidget::setupToolbar()
{
    m_toolbar = new QToolBar();
    m_toolbar->setStyleSheet("QToolBar { border: none; background: #f0f0f0; padding: 3px; spacing: 5px; }");
    m_toolbar->setMaximumHeight(40);

    // Zoom buttons
    QPushButton* zoomInBtn = new QPushButton("🔍+");
    zoomInBtn->setToolTip("Увеличить (Ctrl + колесо мыши)");
    zoomInBtn->setMaximumSize(35, 30);
    connect(zoomInBtn, &QPushButton::clicked, this, &MarkupImageWidget::zoomIn);
    m_toolbar->addWidget(zoomInBtn);

    QPushButton* zoomOutBtn = new QPushButton("🔍-");
    zoomOutBtn->setToolTip("Уменьшить (Ctrl + колесо мыши)");
    zoomOutBtn->setMaximumSize(35, 30);
    connect(zoomOutBtn, &QPushButton::clicked, this, &MarkupImageWidget::zoomOut);
    m_toolbar->addWidget(zoomOutBtn);

    m_toolbar->addSeparator();

    QPushButton* resetBtn = new QPushButton("1:1");
    resetBtn->setToolTip("Исходный размер");
    resetBtn->setMaximumSize(35, 30);
    connect(resetBtn, &QPushButton::clicked, this, &MarkupImageWidget::resetZoom);
    m_toolbar->addWidget(resetBtn);

    QPushButton* fitBtn = new QPushButton("⬜");
    fitBtn->setToolTip("Вписать в окно");
    fitBtn->setMaximumSize(35, 30);
    connect(fitBtn, &QPushButton::clicked, this, &MarkupImageWidget::fitToWindow);
    m_toolbar->addWidget(fitBtn);

    m_toolbar->addSeparator();

    // Zoom slider
    QLabel* zoomLabel = new QLabel(" Масштаб:");
    m_toolbar->addWidget(zoomLabel);

    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(10, 500); // 10% - 500%
    m_zoomSlider->setValue(100);
    m_zoomSlider->setMaximumWidth(150);
    m_zoomSlider->setToolTip("Масштаб изображения");
    connect(m_zoomSlider, &QSlider::valueChanged, this, &MarkupImageWidget::onZoomSliderChanged);
    m_toolbar->addWidget(m_zoomSlider);

    // Zoom spinbox
    m_zoomSpin = new QSpinBox();
    m_zoomSpin->setRange(10, 500);
    m_zoomSpin->setValue(100);
    m_zoomSpin->setSuffix("%");
    m_zoomSpin->setMaximumWidth(80);
    connect(m_zoomSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &MarkupImageWidget::onZoomSpinChanged);
    m_toolbar->addWidget(m_zoomSpin);

    // Spacer
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);
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
    if (!m_updatingControls) {
        double factor = value / 100.0;
        m_imageLabel->setZoomFactor(factor);
    }
}

void MarkupImageWidget::onZoomSpinChanged(int value)
{
    if (!m_updatingControls) {
        double factor = value / 100.0;
        m_imageLabel->setZoomFactor(factor);
    }
}

void MarkupImageWidget::onImageZoomChanged(double factor)
{
    updateZoomControls(factor);
    emit zoomChanged(factor);
}

void MarkupImageWidget::updateZoomControls(double factor)
{
    m_updatingControls = true;

    int percentage = static_cast<int>(factor * 100);
    m_zoomSlider->setValue(percentage);
    m_zoomSpin->setValue(percentage);

    m_updatingControls = false;
}
