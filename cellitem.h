#ifndef CELLITEM_H
#define CELLITEM_H

#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include "cell.h"

class CellItem : public QWidget {
    Q_OBJECT

public:
    explicit CellItem(const Cell& cell, QWidget* parent = nullptr);

    int diameterPx() const;
    double diameterNm() const;
    void setDiameterNm(double nm);
    bool isExcluded() const;
    QString diameterNmText() const;

signals:
    void diameterNmChanged();
    void excludedChanged();

private slots:
    void onDiameterNmEdited(const QString& text);
    void onExcludeClicked();

private:
    Cell m_cell;
    QLabel* imageLabel;
    QLabel* diameterPxLabel;
    QLineEdit* diameterNmEdit;
    QPushButton* excludeButton;
    bool excluded = false;

    void updateImage();
};

#endif // CELLITEM_H
