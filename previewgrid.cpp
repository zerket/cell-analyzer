#include "previewgrid.h"

PreviewGrid::PreviewGrid(QWidget *parent) : QWidget(parent) {
    gridLayout = new QGridLayout(this);
    gridLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    gridLayout->setSpacing(10);
    setLayout(gridLayout);
}

void PreviewGrid::setMaxColumns(int columns) {
    maxColumns = columns;
    rebuildGrid();
}

void PreviewGrid::addPreview(const QString& path) {
    if (imagePaths.contains(path)) return;

    imagePaths.append(path);
    rebuildGrid();
    emit pathsChanged();
}

void PreviewGrid::rebuildGrid() {
    // Очистка сетки
    QLayoutItem* item;
    while ((item = gridLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }

    // Добавление превью заново с правильным позиционированием
    for (int i = 0; i < imagePaths.size(); ++i) {
        const QString& path = imagePaths.at(i);

        QWidget* preview = createPreviewWidget(path);

        int col = maxColumns > 0 ? i % maxColumns : i % 3;
        int row = maxColumns > 0 ? i / maxColumns : i / 3;
        gridLayout->addWidget(preview, row, col);
    }
}

QWidget* PreviewGrid::createPreviewWidget(const QString& path) {
    QWidget* preview = new QWidget;
    preview->setFixedSize(previewSize, previewSize);

    QGridLayout* overlayLayout = new QGridLayout(preview);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);

    QLabel* label = new QLabel;
    label->setPixmap(QPixmap(path).scaled(previewSize, previewSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation));
    label->setAlignment(Qt::AlignCenter);
    label->setFixedSize(previewSize, previewSize);

    overlayLayout->addWidget(label, 0, 0);

    QPushButton* removeBtn = new QPushButton("×");
    removeBtn->setFixedSize(24, 24);
    removeBtn->setStyleSheet(
        "QPushButton {"
        "background-color: rgba(255, 0, 0, 180);"
        "color: white;"
        "border-radius: 12px;"
        "font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "background-color: rgba(255, 0, 0, 230);"
        "}"
        );

    overlayLayout->addWidget(removeBtn, 0, 0, Qt::AlignTop | Qt::AlignRight);

    connect(removeBtn, &QPushButton::clicked, [this, path, preview]() {
        int idx = imagePaths.indexOf(path);
        if (idx != -1) {
            imagePaths.removeAt(idx);
            emit imageRemoved(path);
            emit pathsChanged();

            gridLayout->removeWidget(preview);
            preview->deleteLater();

            rebuildGrid();
        }
    });

    return preview;
}

QStringList PreviewGrid::getPaths() const {
    return imagePaths;
}

void PreviewGrid::setPreviewSize(int size) {
    if (size < 50) size = 50;
    if (size > 300) size = 300;
    if (size != previewSize) {
        previewSize = size;
        rebuildGrid();
    }
}

void PreviewGrid::clearAll() {
    imagePaths.clear();
    rebuildGrid();
    emit pathsChanged();
}
