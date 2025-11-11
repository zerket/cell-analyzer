// markupimagewidget.h - Interactive cell visualization widget
#ifndef MARKUPIMAGEWIDGET_H
#define MARKUPIMAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QPainter>
#include "cell.h"

// Interactive label that handles mouse clicks and drawing
class InteractiveImageLabel : public QLabel {
    Q_OBJECT

public:
    explicit InteractiveImageLabel(QWidget* parent = nullptr);
    void setCells(const QVector<Cell>& cells);
    void setSelectedCell(int index);
    void setOriginalImage(const QPixmap& pixmap);
    void updateDisplay();

signals:
    void cellClicked(int cellIndex);
    void cellRightClicked(int cellIndex);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int findCellAtPosition(const QPoint& pos);

private:
    QVector<Cell> m_cells;
    int m_selectedCellIndex;
    QPixmap m_originalPixmap;
};

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
    InteractiveImageLabel* m_imageLabel;
    QScrollArea* m_scrollArea;
    QPixmap m_currentPixmap;
    QVector<Cell> m_cells;
    int m_selectedCellIndex;
};

#endif // MARKUPIMAGEWIDGET_H
