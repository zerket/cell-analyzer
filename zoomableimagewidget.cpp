#include "zoomableimagewidget.h"
#include "logger.h"
#include <QApplication>
#include <QGraphicsDropShadowEffect>
#include <QSplitter>
#include <cmath>

// ZoomableImageLabel Implementation
ZoomableImageLabel::ZoomableImageLabel(QWidget* parent)
    : QLabel(parent)
    , m_zoomFactor(1.0)
    , m_minZoom(0.1)
    , m_maxZoom(10.0)
    , m_dragging(false)
    , m_panOffset(0, 0)
{
    setAlignment(Qt::AlignCenter);
    setMinimumSize(100, 100);
    setCursor(Qt::OpenHandCursor);
    setStyleSheet("QLabel { border: 1px solid #ddd; background-color: #f9f9f9; }");
}

void ZoomableImageLabel::setPixmap(const QPixmap& pixmap) {
    m_originalPixmap = pixmap;
    updateDisplayedPixmap();
    
    if (!pixmap.isNull()) {
        resize(m_scaledPixmap.size());
    }
}

void ZoomableImageLabel::setZoomFactor(double factor) {
    factor = qBound(m_minZoom, factor, m_maxZoom);
    
    if (qAbs(m_zoomFactor - factor) > 0.001) {
        m_zoomFactor = factor;
        updateDisplayedPixmap();
        emit zoomChanged(m_zoomFactor);
    }
}

void ZoomableImageLabel::zoomIn() {
    setZoomFactor(m_zoomFactor * 1.25);
}

void ZoomableImageLabel::zoomOut() {
    setZoomFactor(m_zoomFactor / 1.25);
}

void ZoomableImageLabel::resetZoom() {
    setZoomFactor(1.0);
    m_panOffset = QPoint(0, 0);
    updateDisplayedPixmap();
}

void ZoomableImageLabel::fitToWindow() {
    if (m_originalPixmap.isNull() || !parentWidget()) return;
    
    QSize parentSize = parentWidget()->size();
    QSize imageSize = m_originalPixmap.size();
    
    double scaleX = double(parentSize.width()) / imageSize.width();
    double scaleY = double(parentSize.height()) / imageSize.height();
    double scale = qMin(scaleX, scaleY) * 0.9; // 90% от размера окна
    
    setZoomFactor(scale);
    m_panOffset = QPoint(0, 0);
}

void ZoomableImageLabel::wheelEvent(QWheelEvent* event) {
    const double scaleFactor = 1.15;
    
    if (event->angleDelta().y() > 0) {
        setZoomFactor(m_zoomFactor * scaleFactor);
    } else {
        setZoomFactor(m_zoomFactor / scaleFactor);
    }
    
    event->accept();
}

void ZoomableImageLabel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastPanPoint = event->pos();
        setCursor(Qt::ClosedHandCursor);
    }
    QLabel::mousePressEvent(event);
}

void ZoomableImageLabel::mouseMoveEvent(QMouseEvent* event) {
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastPanPoint;
        m_panOffset += delta;
        m_lastPanPoint = event->pos();
        updateDisplayedPixmap();
    }
    
    // Отправляем координаты в оригинальном изображении
    QPoint originalPos = mapToOriginalImage(event->pos());
    emit mousePositionChanged(originalPos);
    
    QLabel::mouseMoveEvent(event);
}

void ZoomableImageLabel::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
        setCursor(Qt::OpenHandCursor);
    }
    QLabel::mouseReleaseEvent(event);
}

void ZoomableImageLabel::paintEvent(QPaintEvent* event) {
    QLabel::paintEvent(event);
}

void ZoomableImageLabel::updateDisplayedPixmap() {
    if (m_originalPixmap.isNull()) return;
    
    // Масштабируем изображение
    QSize scaledSize = m_originalPixmap.size() * m_zoomFactor;
    m_scaledPixmap = m_originalPixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    
    // Применяем смещение для панорамирования
    if (!m_panOffset.isNull()) {
        QPixmap offsetPixmap(m_scaledPixmap.size() + QSize(qAbs(m_panOffset.x()) * 2, qAbs(m_panOffset.y()) * 2));
        offsetPixmap.fill(Qt::transparent);
        
        QPainter painter(&offsetPixmap);
        painter.drawPixmap(m_panOffset, m_scaledPixmap);
        
        QLabel::setPixmap(offsetPixmap);
    } else {
        QLabel::setPixmap(m_scaledPixmap);
    }
    
    resize(pixmap().size());
}

QPoint ZoomableImageLabel::mapToOriginalImage(const QPoint& widgetPos) const {
    if (m_originalPixmap.isNull()) return QPoint(-1, -1);
    
    // Учитываем масштаб и смещение
    QPoint adjustedPos = widgetPos - m_panOffset;
    
    double invZoom = 1.0 / m_zoomFactor;
    QPoint originalPos(
        static_cast<int>(adjustedPos.x() * invZoom),
        static_cast<int>(adjustedPos.y() * invZoom)
    );
    
    // Проверяем границы
    if (originalPos.x() < 0 || originalPos.y() < 0 ||
        originalPos.x() >= m_originalPixmap.width() ||
        originalPos.y() >= m_originalPixmap.height()) {
        return QPoint(-1, -1);
    }
    
    return originalPos;
}

// ZoomableImageWidget Implementation
ZoomableImageWidget::ZoomableImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_updatingControls(false)
{
    setupUI();
}

ZoomableImageWidget::~ZoomableImageWidget() {
}

void ZoomableImageWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    setupToolbar();
    mainLayout->addWidget(m_toolbar);
    
    // Создаем область прокрутки с изображением
    m_scrollArea = new QScrollArea();
    m_scrollArea->setWidgetResizable(false);
    m_scrollArea->setAlignment(Qt::AlignCenter);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; }");
    
    m_imageLabel = new ZoomableImageLabel();
    m_scrollArea->setWidget(m_imageLabel);
    
    mainLayout->addWidget(m_scrollArea, 1);
    
    // Подключаем сигналы
    connect(m_imageLabel, &ZoomableImageLabel::zoomChanged, this, &ZoomableImageWidget::onImageZoomChanged);
    connect(m_imageLabel, &ZoomableImageLabel::mousePositionChanged, this, &ZoomableImageWidget::onMousePositionChanged);
}

void ZoomableImageWidget::setupToolbar() {
    m_toolbar = new QToolBar();
    m_toolbar->setStyleSheet("QToolBar { border: none; background: #f0f0f0; padding: 5px; }");
    
    // Кнопки масштабирования
    m_zoomInAction = m_toolbar->addAction("🔍+");
    m_zoomInAction->setToolTip("Увеличить (Ctrl + колесо мыши)");
    connect(m_zoomInAction, &QAction::triggered, this, &ZoomableImageWidget::zoomIn);
    
    m_zoomOutAction = m_toolbar->addAction("🔍-");
    m_zoomOutAction->setToolTip("Уменьшить (Ctrl + колесо мыши)");
    connect(m_zoomOutAction, &QAction::triggered, this, &ZoomableImageWidget::zoomOut);
    
    m_toolbar->addSeparator();
    
    m_resetZoomAction = m_toolbar->addAction("1:1");
    m_resetZoomAction->setToolTip("Исходный размер");
    connect(m_resetZoomAction, &QAction::triggered, this, &ZoomableImageWidget::resetZoom);
    
    m_fitToWindowAction = m_toolbar->addAction("⬜");
    m_fitToWindowAction->setToolTip("Вписать в окно");
    connect(m_fitToWindowAction, &QAction::triggered, this, &ZoomableImageWidget::fitToWindow);
    
    m_toolbar->addSeparator();
    
    // Слайдер масштабирования
    QLabel* zoomLabel = new QLabel("Масштаб:");
    m_toolbar->addWidget(zoomLabel);
    
    m_zoomSlider = new QSlider(Qt::Horizontal);
    m_zoomSlider->setRange(10, 1000); // 10% - 1000%
    m_zoomSlider->setValue(100);
    m_zoomSlider->setMaximumWidth(150);
    m_zoomSlider->setToolTip("Масштаб изображения");
    connect(m_zoomSlider, &QSlider::valueChanged, this, &ZoomableImageWidget::onZoomSliderChanged);
    m_toolbar->addWidget(m_zoomSlider);
    
    // Поле ввода масштаба
    m_zoomSpin = new QSpinBox();
    m_zoomSpin->setRange(10, 1000);
    m_zoomSpin->setValue(100);
    m_zoomSpin->setSuffix("%");
    m_zoomSpin->setMaximumWidth(80);
    connect(m_zoomSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ZoomableImageWidget::onZoomSpinChanged);
    m_toolbar->addWidget(m_zoomSpin);
    
    m_toolbar->addSeparator();
    
    // Информация о позиции мыши
    m_mousePositionLabel = new QLabel("Позиция: —");
    m_mousePositionLabel->setMinimumWidth(120);
    m_toolbar->addWidget(m_mousePositionLabel);
    
    // Информация о размере изображения
    m_imageSizeLabel = new QLabel("Размер: —");
    m_imageSizeLabel->setMinimumWidth(120);
    m_toolbar->addWidget(m_imageSizeLabel);
    
    // m_toolbar->addStretch(); // QToolBar не имеет addStretch, заменим на разделитель
    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_toolbar->addWidget(spacer);
}

void ZoomableImageWidget::setImage(const QPixmap& pixmap) {
    m_imageLabel->setPixmap(pixmap);
    
    if (!pixmap.isNull()) {
        m_imageSizeLabel->setText(QString("Размер: %1×%2").arg(pixmap.width()).arg(pixmap.height()));
        fitToWindow();
    } else {
        m_imageSizeLabel->setText("Размер: —");
        m_mousePositionLabel->setText("Позиция: —");
    }
}

void ZoomableImageWidget::setImage(const QString& imagePath) {
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        Logger::instance().log(QString("Не удалось загрузить изображение: %1").arg(imagePath), LogLevel::WARNING);
    }
    setImage(pixmap);
}

double ZoomableImageWidget::getZoomFactor() const {
    return m_imageLabel->getZoomFactor();
}

void ZoomableImageWidget::setZoomFactor(double factor) {
    m_imageLabel->setZoomFactor(factor);
}

void ZoomableImageWidget::zoomIn() {
    m_imageLabel->zoomIn();
}

void ZoomableImageWidget::zoomOut() {
    m_imageLabel->zoomOut();
}

void ZoomableImageWidget::resetZoom() {
    m_imageLabel->resetZoom();
}

void ZoomableImageWidget::fitToWindow() {
    m_imageLabel->fitToWindow();
}

void ZoomableImageWidget::onZoomSliderChanged(int value) {
    if (!m_updatingControls) {
        double factor = value / 100.0;
        m_imageLabel->setZoomFactor(factor);
    }
}

void ZoomableImageWidget::onZoomSpinChanged(int value) {
    if (!m_updatingControls) {
        double factor = value / 100.0;
        m_imageLabel->setZoomFactor(factor);
    }
}

void ZoomableImageWidget::onImageZoomChanged(double factor) {
    updateZoomControls(factor);
    emit zoomChanged(factor);
}

void ZoomableImageWidget::onMousePositionChanged(QPoint position) {
    if (position.x() >= 0 && position.y() >= 0) {
        m_mousePositionLabel->setText(QString("Позиция: %1, %2").arg(position.x()).arg(position.y()));
    } else {
        m_mousePositionLabel->setText("Позиция: —");
    }
    emit mousePositionChanged(position);
}

void ZoomableImageWidget::updateZoomControls(double factor) {
    m_updatingControls = true;
    
    int percentage = static_cast<int>(factor * 100);
    m_zoomSlider->setValue(percentage);
    m_zoomSpin->setValue(percentage);
    
    m_updatingControls = false;
}