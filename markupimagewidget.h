// markupimagewidget.h - Image viewer with cell markup overlay
#ifndef MARKUPIMAGEWIDGET_H
#define MARKUPIMAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QPixmap>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <opencv2/opencv.hpp>
#include "cell.h"

class MarkupImageLabel : public QLabel {
    Q_OBJECT

public:
    explicit MarkupImageLabel(QWidget* parent = nullptr);

    void setImage(const QString& imagePath);
    void setCells(const QVector<Cell>& cells);
    void setSelectedCell(int cellIndex);
    void clearSelection();

    void setZoomFactor(double factor);
    double getZoomFactor() const { return m_zoomFactor; }

    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

signals:
    void zoomChanged(double factor);
    void cellClicked(int cellIndex);
    void cellRightClicked(int cellIndex);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateDisplayedPixmap();
    void drawCellsOverlay(QPainter& painter);
    int findCellAtPosition(const QPoint& pos);
    QPoint mapToOriginalImage(const QPoint& widgetPos) const;

private:
    QString m_imagePath;
    QPixmap m_originalPixmap;
    QPixmap m_scaledPixmap;
    QVector<Cell> m_cells;
    int m_selectedCellIndex;

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

    void setImage(const QString& imagePath);
    void setCells(const QVector<Cell>& cells);
    void setSelectedCell(int cellIndex);
    void clearSelection();

    double getZoomFactor() const;
    void setZoomFactor(double factor);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

signals:
    void zoomChanged(double factor);
    void cellClicked(int cellIndex);
    void cellRightClicked(int cellIndex);

private slots:
    void onZoomSliderChanged(int value);
    void onZoomSpinChanged(int value);
    void onImageZoomChanged(double factor);

private:
    void setupUI();
    void setupToolbar();
    void updateZoomControls(double factor);

private:
    MarkupImageLabel* m_imageLabel;
    QScrollArea* m_scrollArea;

    // Zoom controls
    QPushButton* m_zoomInButton;
    QPushButton* m_zoomOutButton;
    QPushButton* m_resetZoomButton;
    QPushButton* m_fitWindowButton;
    QSlider* m_zoomSlider;
    QSpinBox* m_zoomSpin;

    bool m_updatingControls;
};

#endif // MARKUPIMAGEWIDGET_H
