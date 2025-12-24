// markupimagewidget.h - Interactive cell visualization widget
#ifndef MARKUPIMAGEWIDGET_H
#define MARKUPIMAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QPixmap>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QToolBar>
#include <QSlider>
#include <QSpinBox>
#include <QPushButton>
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

    // Zoom functionality
    void setZoomFactor(double factor);
    double getZoomFactor() const { return m_zoomFactor; }
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

signals:
    void cellClicked(int cellIndex);
    void cellRightClicked(int cellIndex);
    void zoomChanged(double factor);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    int findCellAtPosition(const QPoint& pos);
    QPoint mapToOriginalImage(const QPoint& widgetPos) const;

private:
    QVector<Cell> m_cells;
    int m_selectedCellIndex;
    QPixmap m_originalPixmap;
    int m_canvasOffset = 0;  // Offset for extended canvas

    // Zoom and pan
    double m_zoomFactor;
    double m_minZoom;
    double m_maxZoom;
    bool m_dragging;
    QPoint m_lastPanPoint;
    QPoint m_panOffset;
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

    // Zoom control
    double getZoomFactor() const;
    void setZoomFactor(double factor);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

signals:
    void cellClicked(int cellIndex);
    void cellRightClicked(int cellIndex);
    void zoomChanged(double factor);

private slots:
    void onZoomSliderChanged(int value);
    void onZoomSpinChanged(int value);
    void onImageZoomChanged(double factor);

private:
    void setupUI();
    void setupToolbar();
    void updateZoomControls(double factor);

private:
    InteractiveImageLabel* m_imageLabel;
    QScrollArea* m_scrollArea;
    QPixmap m_currentPixmap;
    QVector<Cell> m_cells;
    int m_selectedCellIndex;

    // Toolbar and controls
    QToolBar* m_toolbar;
    QSlider* m_zoomSlider;
    QSpinBox* m_zoomSpin;
    bool m_updatingControls;
};

#endif // MARKUPIMAGEWIDGET_H
