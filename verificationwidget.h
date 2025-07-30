// verificationwidget.h
#ifndef VERIFICATIONWIDGET_H
#define VERIFICATIONWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include "cell.h"
#include "cellitemwidget.h"

class VerificationWidget : public QWidget {
    Q_OBJECT

public:
    explicit VerificationWidget(const QVector<Cell>& cells, QWidget *parent = nullptr);

signals:
    void analysisCompleted();

private slots:
    void onDiameterNmChanged();
    void onRecalculateClicked();
    void onRemoveCellRequested(CellItemWidget* item);
    void onClearDiametersClicked();

private:
    QListWidget* listWidget;
    QVector<Cell> m_cells;
    QPushButton* finishButton;
    QPushButton* recalcButton;
    QPushButton* saveButton;
    QPushButton* clearDiametersButton;

    void onSaveCellsClicked();
    void updateRecalcButtonState();
    void recalculateDiameters();
};

#endif // VERIFICATIONWIDGET_H
