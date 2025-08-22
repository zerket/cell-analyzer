// verificationwidget.h - IMPROVED VERSION
#ifndef VERIFICATIONWIDGET_H
#define VERIFICATIONWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QButtonGroup>
#include <QRadioButton>
#include <QScrollArea>
#include <QLineEdit>
#include <QTimer>
#include <QPointer>
#include "cell.h"
#include "cellitemwidget.h"

class VerificationWidget : public QWidget {
    Q_OBJECT

public:
    explicit VerificationWidget(const QVector<Cell>& cells, QWidget *parent = nullptr);
    ~VerificationWidget();

signals:
    void analysisCompleted();

private slots:
    void onDiameterNmChanged();
    void onRecalculateClicked();
    void onRemoveCellRequested(CellItemWidget* item);
    void onRemoveCellAtIndex(int index);
    void onClearDiametersClicked();
    void onViewModeChanged(int mode);
    void onSaveCellsClicked();
    void performDelayedResize();

private:
    // Setup methods
    void setupUI();
    void setupGridView();
    void setupListView();
    void cleanupWidgets();
    void loadSavedCoefficient();
    
    // Helper methods
    void updateRecalcButtonState();
    cv::Mat loadImageSafely(const QString& imagePath);
    void recalculateDiameters();
    void saveDebugImage(const QString& originalImagePath, 
                       const QVector<QPair<Cell, double>>& cells,
                       const QString& outputPath);
    
    // UI Elements
    QListWidget* listWidget;
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
    
    // Data
    QVector<Cell> m_cells;
    QVector<QPointer<CellItemWidget>> m_cellWidgets; // Use QPointer for safety
    QVector<QLineEdit*> listViewDiameterEdits;
    
    // Optimization
    QTimer* m_resizeTimer;
    
protected:
    void resizeEvent(QResizeEvent* event) override;
};

#endif // VERIFICATIONWIDGET_H