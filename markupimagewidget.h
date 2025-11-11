// markupimagewidget.h - Simplified stub for YOLO version
#ifndef MARKUPIMAGEWIDGET_H
#define MARKUPIMAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QScrollArea>
#include "cell.h"

// Simplified MarkupImageWidget - displays image without markup functionality
// This is a stub replacement after YOLO migration removed the original implementation
class MarkupImageWidget : public QWidget {
    Q_OBJECT

public:
    explicit MarkupImageWidget(QWidget* parent = nullptr);
    ~MarkupImageWidget();

    void setImage(const QPixmap& pixmap);
    void setImage(const QString& imagePath);
    void setCells(const QVector<Cell>& cells);
    void setSelectedCell(int index);
    void clear();

signals:
    void cellClicked(int cellIndex);
    void cellRightClicked(int cellIndex);

private:
    QLabel* m_imageLabel;
    QScrollArea* m_scrollArea;
    QPixmap m_currentPixmap;
    QVector<Cell> m_cells;
    int m_selectedCellIndex;
};

#endif // MARKUPIMAGEWIDGET_H
