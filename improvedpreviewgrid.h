#ifndef IMPROVEDPREVIEWGRID_H
#define IMPROVEDPREVIEWGRID_H

#include <QWidget>
#include <QGridLayout>
#include <QLabel>
#include <QPixmap>
#include <QStringList>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QFileInfo>
#include <QMenu>
#include <QContextMenuEvent>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>

class PreviewImageWidget : public QLabel {
    Q_OBJECT

public:
    explicit PreviewImageWidget(const QString& imagePath, int size = 150, QWidget* parent = nullptr);
    
    void setImagePath(const QString& path);
    QString getImagePath() const { return m_imagePath; }
    
    void setPreviewSize(int size);
    int getPreviewSize() const { return m_previewSize; }
    
    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }
    
    void setHighlighted(bool highlighted);

signals:
    void clicked(PreviewImageWidget* widget);
    void doubleClicked(PreviewImageWidget* widget);
    void contextMenuRequested(PreviewImageWidget* widget, const QPoint& position);
    void removeRequested(PreviewImageWidget* widget);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
    void paintEvent(QPaintEvent* event) override;

private:
    void updatePixmap();
    void animateHighlight(bool highlight);
    
private:
    QString m_imagePath;
    QPixmap m_originalPixmap;
    int m_previewSize;
    bool m_selected;
    bool m_highlighted;
    QPropertyAnimation* m_animation;
    QGraphicsOpacityEffect* m_opacityEffect;
};

class ImprovedPreviewGrid : public QWidget {
    Q_OBJECT

public:
    explicit ImprovedPreviewGrid(QWidget* parent = nullptr);
    ~ImprovedPreviewGrid();
    
    void addImages(const QStringList& imagePaths);
    void addImage(const QString& imagePath);
    void removeImage(const QString& imagePath);
    void removeSelectedImages();
    void clear();
    
    QStringList getImagePaths() const;
    QStringList getSelectedImagePaths() const;
    int getImageCount() const { return m_imagePaths.size(); }
    
    void setPreviewSize(int size);
    int getPreviewSize() const { return m_previewSize; }
    
    void setMaxColumns(int maxColumns);
    int getMaxColumns() const { return m_maxColumns; }
    
    void setSelectionMode(bool multiSelection) { m_multiSelection = multiSelection; }
    bool isMultiSelectionEnabled() const { return m_multiSelection; }

signals:
    void imageAdded(const QString& imagePath);
    void imageRemoved(const QString& imagePath);
    void selectionChanged(const QStringList& selectedPaths);
    void imageDoubleClicked(const QString& imagePath);
    void imagesDropped(const QStringList& imagePaths);

public slots:
    void selectAll();
    void selectNone();
    void selectInvert();

private slots:
    void onImageClicked(PreviewImageWidget* widget);
    void onImageDoubleClicked(PreviewImageWidget* widget);
    void onImageContextMenu(PreviewImageWidget* widget, const QPoint& position);
    void onImageRemoveRequested(PreviewImageWidget* widget);
    void onPreviewSizeChanged(int size);
    void onSortModeChanged(int mode);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void setupUI();
    void setupToolbar();
    void updateLayout();
    void updateSelection();
    void showDropZone(bool show);
    bool isImageFile(const QString& filePath) const;
    
    enum SortMode {
        SortByName,
        SortByDate,
        SortBySize
    };
    
    void sortImages(SortMode mode);
    
private:
    // UI элементы
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_toolbarLayout;
    QScrollArea* m_scrollArea;
    QWidget* m_gridWidget;
    QGridLayout* m_gridLayout;
    QLabel* m_dropZoneLabel;
    
    // Toolbar контролы
    QSlider* m_sizeSlider;
    QSpinBox* m_sizeSpin;
    QPushButton* m_selectAllButton;
    QPushButton* m_selectNoneButton;
    QPushButton* m_removeSelectedButton;
    QComboBox* m_sortCombo;
    QLabel* m_infoLabel;
    
    // Данные
    QStringList m_imagePaths;
    QList<PreviewImageWidget*> m_widgets;
    QList<PreviewImageWidget*> m_selectedWidgets;
    
    // Настройки
    int m_previewSize;
    int m_maxColumns;
    bool m_multiSelection;
    SortMode m_currentSortMode;
};

#endif // IMPROVEDPREVIEWGRID_H