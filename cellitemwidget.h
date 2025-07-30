// cellitemwidget.h
#ifndef CELLITEMWIDGET_H
#define CELLITEMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QPushButton>
#include "cell.h"

class CellItemWidget : public QWidget {
    Q_OBJECT
public:
    explicit CellItemWidget(const Cell& cell, QWidget* parent = nullptr);

    QString diameterNmText() const;
    int diameterPx() const;
    void setDiameterNm(double nm);
    QImage getImage() const;

signals:
    void diameterNmChanged();
    void excludedChanged();
    void removeRequested(CellItemWidget* item);  // сигнал на удаление

private:
    QLabel* imageLabel;
    QLabel* diameterPxLabel;
    QLineEdit* diameterNmEdit;
    QPushButton* removeButton;

    Cell m_cell;
};

#endif // CELLITEMWIDGET_H
