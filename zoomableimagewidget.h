#ifndef ZOOMABLEIMAGEWIDGET_H
#define ZOOMABLEIMAGEWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QScrollArea>
#include <QPixmap>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPoint>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSlider>
#include <QSpinBox>
#include <QToolBar>
#include <QAction>
#include <QPainter>

class ZoomableImageLabel : public QLabel {
    Q_OBJECT

public:
    explicit ZoomableImageLabel(QWidget* parent = nullptr);
    
    void setPixmap(const QPixmap& pixmap);
    void setZoomFactor(double factor);
    double getZoomFactor() const { return m_zoomFactor; }
    
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

signals:
    void zoomChanged(double factor);
    void mousePositionChanged(QPoint position);

protected:
    void wheelEvent(QWheelEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updateDisplayedPixmap();
    QPoint mapToOriginalImage(const QPoint& widgetPos) const;
    
private:
    QPixmap m_originalPixmap;
    QPixmap m_scaledPixmap;
    double m_zoomFactor;
    double m_minZoom;
    double m_maxZoom;
    
    bool m_dragging;
    QPoint m_lastPanPoint;
    QPoint m_panOffset;
};

class ZoomableImageWidget : public QWidget {
    Q_OBJECT

public:
    explicit ZoomableImageWidget(QWidget* parent = nullptr);
    ~ZoomableImageWidget();
    
    void setImage(const QPixmap& pixmap);
    void setImage(const QString& imagePath);
    
    double getZoomFactor() const;
    void setZoomFactor(double factor);

public slots:
    void zoomIn();
    void zoomOut();
    void resetZoom();
    void fitToWindow();

signals:
    void zoomChanged(double factor);
    void mousePositionChanged(QPoint imagePosition);

private slots:
    void onZoomSliderChanged(int value);
    void onZoomSpinChanged(int value);
    void onImageZoomChanged(double factor);
    void onMousePositionChanged(QPoint position);

private:
    void setupUI();
    void setupToolbar();
    void updateZoomControls(double factor);
    
private:
    ZoomableImageLabel* m_imageLabel;
    QScrollArea* m_scrollArea;
    
    // Toolbar и контролы
    QToolBar* m_toolbar;
    QAction* m_zoomInAction;
    QAction* m_zoomOutAction;
    QAction* m_resetZoomAction;
    QAction* m_fitToWindowAction;
    
    QSlider* m_zoomSlider;
    QSpinBox* m_zoomSpin;
    QLabel* m_mousePositionLabel;
    QLabel* m_imageSizeLabel;
    
    bool m_updatingControls;
};

#endif // ZOOMABLEIMAGEWIDGET_H