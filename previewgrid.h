#ifndef PREVIEWGRID_H
#define PREVIEWGRID_H

#include <QWidget>
#include <QStringList>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>

class PreviewGrid : public QWidget {
    Q_OBJECT

public:
    explicit PreviewGrid(QWidget *parent = nullptr);
    void addPreview(const QString& path);
    void rebuildGrid();
    QStringList getPaths() const;
    void setMaxColumns(int columns);

signals:
    void imageRemoved(const QString& path);
    void pathsChanged();

private:
    QWidget* createPreviewWidget(const QString& path);  // новый приватный метод

    QGridLayout* gridLayout;
    QStringList imagePaths;
    int maxColumns = 3;
};

#endif // PREVIEWGRID_H
