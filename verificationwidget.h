// verificationwidget.h
#ifndef VERIFICATIONWIDGET_H
#define VERIFICATIONWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QScrollArea>
#include <QLineEdit>
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
    void onViewModeChanged(int mode);

private:
    QListWidget* listWidget;
    QVector<Cell> m_cells;
    QPushButton* finishButton;
    QPushButton* recalcButton;
    QPushButton* saveButton;
    QPushButton* clearDiametersButton;
    QRadioButton* gridViewButton;
    QRadioButton* listViewButton;
    QButtonGroup* viewModeGroup;
    QScrollArea* scrollArea;
    QWidget* cellsContainer;
    QLineEdit* coefficientEdit;
    QVector<QLineEdit*> listViewDiameterEdits;

    void onSaveCellsClicked();
    void updateRecalcButtonState();
    void recalculateDiameters();
    void setupGridView();
    void setupListView();
    void saveDebugImage(const QString& originalImagePath, 
                       const QVector<QPair<Cell, double>>& cells,
                       const QString& outputPath);
                       
protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // VERIFICATIONWIDGET_H
