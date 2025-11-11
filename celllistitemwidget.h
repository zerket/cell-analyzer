// celllistitemwidget.h - Compact cell list item for verification widget
#ifndef CELLLISTITEMWIDGET_H
#define CELLLISTITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QMouseEvent>
#include "cell.h"

class CellListItemWidget : public QWidget {
    Q_OBJECT

public:
    explicit CellListItemWidget(int cellNumber, const Cell& cell, QWidget* parent = nullptr);

    // Getters
    int cellNumber() const { return m_cellNumber; }
    const Cell& cell() const { return m_cell; }
    QString diameterNmText() const;
    double getDiameterNm() const;
    int diameterPx() const { return m_cell.diameterPx; }

    // Setters
    void setDiameterNm(double nm);
    void clearDiameterNm();
    void setSelected(bool selected);

    // Lazy loading for thumbnail
    void loadThumbnail();

signals:
    void clicked(CellListItemWidget* item);
    void diameterNmChanged();
    void removeRequested(CellListItemWidget* item);

protected:
    void mousePressEvent(QMouseEvent* event) override;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    void enterEvent(QEnterEvent* event) override;
#else
    void enterEvent(QEvent* event) override;
#endif
    void leaveEvent(QEvent* event) override;

private:
    void setupUI();
    void updateStyle();

private:
    int m_cellNumber;
    Cell m_cell;
    bool m_selected;
    bool m_hovered;
    bool m_thumbnailLoaded;

    QLabel* m_numberLabel;
    QLabel* m_imageLabel;
    QLabel* m_diameterPxLabel;
    QLineEdit* m_diameterNmEdit;
    QPushButton* m_removeButton;
};

#endif // CELLLISTITEMWIDGET_H
