#include "improvedpreviewgrid.h"
#include <QFileInfo>
#include <QDateTime>
#include <QDir>
#include <QPainter>
#include <QMouseEvent>
#include <QTimer>
#include <algorithm>

// ============================================================================
// PreviewImageWidget Implementation
// ============================================================================

PreviewImageWidget::PreviewImageWidget(const QString& imagePath, int size, QWidget* parent)
    : QLabel(parent)
    , m_imagePath(imagePath)
    , m_previewSize(size)
    , m_selected(false)
    , m_highlighted(false)
    , m_animation(nullptr)
    , m_opacityEffect(nullptr)
{
    setFixedSize(m_previewSize, m_previewSize);
    setAlignment(Qt::AlignCenter);
    setFrameStyle(QFrame::Box);
    setLineWidth(2);
    setCursor(Qt::PointingHandCursor);
    setAcceptDrops(false);

    // Load and set pixmap
    updatePixmap();

    // Setup opacity effect for animations
    m_opacityEffect = new QGraphicsOpacityEffect(this);
    m_opacityEffect->setOpacity(1.0);
    setGraphicsEffect(m_opacityEffect);

    m_animation = new QPropertyAnimation(m_opacityEffect, "opacity", this);
    m_animation->setDuration(200);
}

void PreviewImageWidget::setImagePath(const QString& path) {
    if (m_imagePath != path) {
        m_imagePath = path;
        updatePixmap();
    }
}

void PreviewImageWidget::setPreviewSize(int size) {
    if (m_previewSize != size) {
        m_previewSize = size;
        setFixedSize(m_previewSize, m_previewSize);
        updatePixmap();
    }
}

void PreviewImageWidget::setSelected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void PreviewImageWidget::setHighlighted(bool highlighted) {
    if (m_highlighted != highlighted) {
        m_highlighted = highlighted;
        animateHighlight(highlighted);
        update();
    }
}

void PreviewImageWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked(this);
    }
    QLabel::mousePressEvent(event);
}

void PreviewImageWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit doubleClicked(this);
    }
    QLabel::mouseDoubleClickEvent(event);
}

void PreviewImageWidget::contextMenuEvent(QContextMenuEvent* event) {
    emit contextMenuRequested(this, event->pos());
    event->accept();
}

void PreviewImageWidget::enterEvent(QEnterEvent* event) {
    setHighlighted(true);
    QLabel::enterEvent(event);
}

void PreviewImageWidget::leaveEvent(QEvent* event) {
    setHighlighted(false);
    QLabel::leaveEvent(event);
}

void PreviewImageWidget::paintEvent(QPaintEvent* event) {
    QLabel::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // Draw selection border
    if (m_selected) {
        QPen pen(QColor(33, 150, 243), 3); // Blue border
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(1, 1, -2, -2));
    } else if (m_highlighted) {
        QPen pen(QColor(100, 100, 100), 2); // Gray border on hover
        painter.setPen(pen);
        painter.drawRect(rect().adjusted(1, 1, -2, -2));
    }
}

void PreviewImageWidget::updatePixmap() {
    m_originalPixmap = QPixmap(m_imagePath);
    if (!m_originalPixmap.isNull()) {
        QPixmap scaled = m_originalPixmap.scaled(
            m_previewSize - 4, m_previewSize - 4,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        );
        setPixmap(scaled);
    }
}

void PreviewImageWidget::animateHighlight(bool highlight) {
    if (m_animation) {
        m_animation->stop();
        m_animation->setStartValue(m_opacityEffect->opacity());
        m_animation->setEndValue(highlight ? 0.8 : 1.0);
        m_animation->start();
    }
}

// ============================================================================
// ImprovedPreviewGrid Implementation
// ============================================================================

ImprovedPreviewGrid::ImprovedPreviewGrid(QWidget* parent)
    : QWidget(parent)
    , m_previewSize(150)
    , m_maxColumns(4)
    , m_multiSelection(true)
    , m_currentSortMode(SortByName)
{
    setupUI();
    setAcceptDrops(true);
}

ImprovedPreviewGrid::~ImprovedPreviewGrid() {
    clear();
}

void ImprovedPreviewGrid::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(5);

    // Setup toolbar
    setupToolbar();

    // Create scroll area
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Create grid widget
    m_gridWidget = new QWidget();
    m_gridLayout = new QGridLayout(m_gridWidget);
    m_gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_gridLayout->setSpacing(10);
    m_gridWidget->setLayout(m_gridLayout);

    m_scrollArea->setWidget(m_gridWidget);
    m_mainLayout->addWidget(m_scrollArea);

    // Create drop zone label (initially hidden)
    m_dropZoneLabel = new QLabel("Перетащите изображения сюда", this);
    m_dropZoneLabel->setAlignment(Qt::AlignCenter);
    m_dropZoneLabel->setStyleSheet(
        "QLabel { "
        "background-color: rgba(33, 150, 243, 50); "
        "border: 2px dashed #2196F3; "
        "border-radius: 10px; "
        "color: #2196F3; "
        "font-size: 16px; "
        "padding: 20px; "
        "}"
    );
    m_dropZoneLabel->hide();

    setLayout(m_mainLayout);
}

void ImprovedPreviewGrid::setupToolbar() {
    m_toolbarLayout = new QHBoxLayout();
    m_toolbarLayout->setSpacing(10);

    // Preview size slider
    QLabel* sizeLabel = new QLabel("Размер:");
    m_toolbarLayout->addWidget(sizeLabel);

    m_sizeSlider = new QSlider(Qt::Horizontal);
    m_sizeSlider->setRange(100, 500);
    m_sizeSlider->setValue(m_previewSize);
    m_sizeSlider->setFixedWidth(150);
    m_toolbarLayout->addWidget(m_sizeSlider);

    m_sizeSpin = new QSpinBox();
    m_sizeSpin->setRange(100, 500);
    m_sizeSpin->setValue(m_previewSize);
    m_sizeSpin->setSuffix(" px");
    m_toolbarLayout->addWidget(m_sizeSpin);

    // Connect size controls
    connect(m_sizeSlider, &QSlider::valueChanged, this, &ImprovedPreviewGrid::onPreviewSizeChanged);
    connect(m_sizeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImprovedPreviewGrid::onPreviewSizeChanged);

    m_toolbarLayout->addSpacing(20);

    // Sort combo
    QLabel* sortLabel = new QLabel("Сортировка:");
    m_toolbarLayout->addWidget(sortLabel);

    m_sortCombo = new QComboBox();
    m_sortCombo->addItem("По имени", SortByName);
    m_sortCombo->addItem("По дате", SortByDate);
    m_sortCombo->addItem("По размеру", SortBySize);
    m_toolbarLayout->addWidget(m_sortCombo);

    connect(m_sortCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImprovedPreviewGrid::onSortModeChanged);

    m_toolbarLayout->addSpacing(20);

    // Selection buttons
    m_selectAllButton = new QPushButton("Выбрать все");
    m_selectAllButton->setEnabled(false);
    connect(m_selectAllButton, &QPushButton::clicked, this, &ImprovedPreviewGrid::selectAll);
    m_toolbarLayout->addWidget(m_selectAllButton);

    m_selectNoneButton = new QPushButton("Снять выделение");
    m_selectNoneButton->setEnabled(false);
    connect(m_selectNoneButton, &QPushButton::clicked, this, &ImprovedPreviewGrid::selectNone);
    m_toolbarLayout->addWidget(m_selectNoneButton);

    m_removeSelectedButton = new QPushButton("Удалить выбранные");
    m_removeSelectedButton->setEnabled(false);
    connect(m_removeSelectedButton, &QPushButton::clicked, this, &ImprovedPreviewGrid::removeSelectedImages);
    m_toolbarLayout->addWidget(m_removeSelectedButton);

    m_toolbarLayout->addStretch();

    // Info label
    m_infoLabel = new QLabel("Изображений: 0");
    m_toolbarLayout->addWidget(m_infoLabel);

    m_mainLayout->addLayout(m_toolbarLayout);
}

void ImprovedPreviewGrid::addImages(const QStringList& imagePaths) {
    for (const QString& path : imagePaths) {
        addImage(path);
    }
}

void ImprovedPreviewGrid::addImage(const QString& imagePath) {
    if (m_imagePaths.contains(imagePath)) {
        return;
    }

    if (!isImageFile(imagePath)) {
        return;
    }

    m_imagePaths.append(imagePath);

    // Sort if needed
    sortImages(m_currentSortMode);

    // Create widget
    PreviewImageWidget* widget = new PreviewImageWidget(imagePath, m_previewSize, m_gridWidget);
    m_widgets.append(widget);

    // Connect signals
    connect(widget, &PreviewImageWidget::clicked, this, &ImprovedPreviewGrid::onImageClicked);
    connect(widget, &PreviewImageWidget::doubleClicked, this, &ImprovedPreviewGrid::onImageDoubleClicked);
    connect(widget, &PreviewImageWidget::contextMenuRequested, this, &ImprovedPreviewGrid::onImageContextMenu);
    connect(widget, &PreviewImageWidget::removeRequested, this, &ImprovedPreviewGrid::onImageRemoveRequested);

    updateLayout();
    updateSelection();

    // Update info
    m_infoLabel->setText(QString("Изображений: %1").arg(m_imagePaths.size()));
    m_selectAllButton->setEnabled(m_imagePaths.size() > 0);

    emit imageAdded(imagePath);
}

void ImprovedPreviewGrid::removeImage(const QString& imagePath) {
    int index = m_imagePaths.indexOf(imagePath);
    if (index == -1) {
        return;
    }

    m_imagePaths.removeAt(index);

    if (index < m_widgets.size()) {
        PreviewImageWidget* widget = m_widgets.takeAt(index);
        m_selectedWidgets.removeAll(widget);
        m_gridLayout->removeWidget(widget);
        widget->deleteLater();
    }

    updateLayout();
    updateSelection();

    m_infoLabel->setText(QString("Изображений: %1").arg(m_imagePaths.size()));
    m_selectAllButton->setEnabled(m_imagePaths.size() > 0);

    emit imageRemoved(imagePath);
}

void ImprovedPreviewGrid::removeSelectedImages() {
    QStringList pathsToRemove;
    for (PreviewImageWidget* widget : m_selectedWidgets) {
        pathsToRemove.append(widget->getImagePath());
    }

    for (const QString& path : pathsToRemove) {
        removeImage(path);
    }
}

void ImprovedPreviewGrid::clear() {
    m_imagePaths.clear();
    m_selectedWidgets.clear();

    for (PreviewImageWidget* widget : m_widgets) {
        m_gridLayout->removeWidget(widget);
        widget->deleteLater();
    }
    m_widgets.clear();

    m_infoLabel->setText("Изображений: 0");
    m_selectAllButton->setEnabled(false);
    m_selectNoneButton->setEnabled(false);
    m_removeSelectedButton->setEnabled(false);
}

QStringList ImprovedPreviewGrid::getImagePaths() const {
    return m_imagePaths;
}

QStringList ImprovedPreviewGrid::getSelectedImagePaths() const {
    QStringList paths;
    for (PreviewImageWidget* widget : m_selectedWidgets) {
        paths.append(widget->getImagePath());
    }
    return paths;
}

void ImprovedPreviewGrid::setPreviewSize(int size) {
    if (m_previewSize != size && size >= 100 && size <= 500) {
        m_previewSize = size;

        m_sizeSlider->blockSignals(true);
        m_sizeSpin->blockSignals(true);

        m_sizeSlider->setValue(size);
        m_sizeSpin->setValue(size);

        m_sizeSlider->blockSignals(false);
        m_sizeSpin->blockSignals(false);

        for (PreviewImageWidget* widget : m_widgets) {
            widget->setPreviewSize(size);
        }

        updateLayout();
    }
}

void ImprovedPreviewGrid::setMaxColumns(int maxColumns) {
    if (m_maxColumns != maxColumns && maxColumns > 0) {
        m_maxColumns = maxColumns;
        updateLayout();
    }
}

void ImprovedPreviewGrid::selectAll() {
    m_selectedWidgets = m_widgets;
    for (PreviewImageWidget* widget : m_widgets) {
        widget->setSelected(true);
    }
    updateSelection();
}

void ImprovedPreviewGrid::selectNone() {
    for (PreviewImageWidget* widget : m_selectedWidgets) {
        widget->setSelected(false);
    }
    m_selectedWidgets.clear();
    updateSelection();
}

void ImprovedPreviewGrid::selectInvert() {
    QList<PreviewImageWidget*> newSelection;

    for (PreviewImageWidget* widget : m_widgets) {
        if (m_selectedWidgets.contains(widget)) {
            widget->setSelected(false);
        } else {
            widget->setSelected(true);
            newSelection.append(widget);
        }
    }

    m_selectedWidgets = newSelection;
    updateSelection();
}

void ImprovedPreviewGrid::onImageClicked(PreviewImageWidget* widget) {
    if (!m_multiSelection) {
        // Single selection mode
        selectNone();
        widget->setSelected(true);
        m_selectedWidgets.append(widget);
    } else {
        // Multi selection mode
        bool isSelected = m_selectedWidgets.contains(widget);
        if (isSelected) {
            widget->setSelected(false);
            m_selectedWidgets.removeAll(widget);
        } else {
            widget->setSelected(true);
            m_selectedWidgets.append(widget);
        }
    }

    updateSelection();
}

void ImprovedPreviewGrid::onImageDoubleClicked(PreviewImageWidget* widget) {
    emit imageDoubleClicked(widget->getImagePath());
}

void ImprovedPreviewGrid::onImageContextMenu(PreviewImageWidget* widget, const QPoint& position) {
    QMenu menu(this);

    QAction* removeAction = menu.addAction("Удалить");
    connect(removeAction, &QAction::triggered, [this, widget]() {
        removeImage(widget->getImagePath());
    });

    menu.addSeparator();

    QAction* selectAction = menu.addAction(widget->isSelected() ? "Снять выделение" : "Выбрать");
    connect(selectAction, &QAction::triggered, [this, widget]() {
        onImageClicked(widget);
    });

    menu.exec(widget->mapToGlobal(position));
}

void ImprovedPreviewGrid::onImageRemoveRequested(PreviewImageWidget* widget) {
    removeImage(widget->getImagePath());
}

void ImprovedPreviewGrid::onPreviewSizeChanged(int size) {
    setPreviewSize(size);
}

void ImprovedPreviewGrid::onSortModeChanged(int mode) {
    m_currentSortMode = static_cast<SortMode>(m_sortCombo->currentData().toInt());
    sortImages(m_currentSortMode);
    updateLayout();
}

void ImprovedPreviewGrid::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        showDropZone(true);
    }
}

void ImprovedPreviewGrid::dropEvent(QDropEvent* event) {
    showDropZone(false);

    QStringList imagePaths;
    const QMimeData* mimeData = event->mimeData();

    if (mimeData->hasUrls()) {
        for (const QUrl& url : mimeData->urls()) {
            QString path = url.toLocalFile();
            if (isImageFile(path)) {
                imagePaths.append(path);
            }
        }
    }

    if (!imagePaths.isEmpty()) {
        addImages(imagePaths);
        emit imagesDropped(imagePaths);
    }

    event->acceptProposedAction();
}

void ImprovedPreviewGrid::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateLayout();
}

void ImprovedPreviewGrid::updateLayout() {
    // Clear layout
    for (int i = 0; i < m_widgets.size(); ++i) {
        m_gridLayout->removeWidget(m_widgets[i]);
    }

    // Re-add widgets in grid
    for (int i = 0; i < m_widgets.size(); ++i) {
        int row = i / m_maxColumns;
        int col = i % m_maxColumns;
        m_gridLayout->addWidget(m_widgets[i], row, col);
    }
}

void ImprovedPreviewGrid::updateSelection() {
    m_selectNoneButton->setEnabled(!m_selectedWidgets.isEmpty());
    m_removeSelectedButton->setEnabled(!m_selectedWidgets.isEmpty());

    emit selectionChanged(getSelectedImagePaths());
}

void ImprovedPreviewGrid::showDropZone(bool show) {
    if (show && m_imagePaths.isEmpty()) {
        m_dropZoneLabel->show();
        m_dropZoneLabel->raise();
    } else {
        m_dropZoneLabel->hide();
    }
}

bool ImprovedPreviewGrid::isImageFile(const QString& filePath) const {
    QStringList imageExtensions = {"jpg", "jpeg", "png", "bmp", "gif", "tiff", "tif"};
    QFileInfo fileInfo(filePath);
    return imageExtensions.contains(fileInfo.suffix().toLower());
}

void ImprovedPreviewGrid::sortImages(SortMode mode) {
    QList<QPair<QString, QFileInfo>> fileInfoList;

    for (const QString& path : m_imagePaths) {
        fileInfoList.append(qMakePair(path, QFileInfo(path)));
    }

    switch (mode) {
        case SortByName:
            std::sort(fileInfoList.begin(), fileInfoList.end(),
                [](const QPair<QString, QFileInfo>& a, const QPair<QString, QFileInfo>& b) {
                    return a.second.fileName() < b.second.fileName();
                });
            break;

        case SortByDate:
            std::sort(fileInfoList.begin(), fileInfoList.end(),
                [](const QPair<QString, QFileInfo>& a, const QPair<QString, QFileInfo>& b) {
                    return a.second.lastModified() > b.second.lastModified();
                });
            break;

        case SortBySize:
            std::sort(fileInfoList.begin(), fileInfoList.end(),
                [](const QPair<QString, QFileInfo>& a, const QPair<QString, QFileInfo>& b) {
                    return a.second.size() > b.second.size();
                });
            break;
    }

    m_imagePaths.clear();
    for (const auto& pair : fileInfoList) {
        m_imagePaths.append(pair.first);
    }
}
