// verificationwidget.h - NEW DESIGN VERSION (Variant 5)
#ifndef VERIFICATIONWIDGET_H
#define VERIFICATIONWIDGET_H

#include <QWidget>
#include <QTabWidget>
#include <QSplitter>
#include <QScrollArea>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QMap>
#include "cell.h"
#include "celllistitemwidget.h"
#include "markupimagewidget.h"

class VerificationWidget : public QWidget {
    Q_OBJECT

public:
    explicit VerificationWidget(const QVector<Cell>& cells, QWidget *parent = nullptr);
    ~VerificationWidget();

    QVector<Cell> getVerifiedCells() const;

signals:
    void analysisCompleted();
    void statisticsRequested();

private slots:
    void onFileTabChanged(int index);
    void onCellItemClicked(CellListItemWidget* item);
    void onCellItemRemoved(CellListItemWidget* item);
    void onImageCellClicked(int cellIndex);
    void onDiameterNmChanged();
    void onRecalculateClicked();
    void onClearDiametersClicked();
    void onSaveCellsClicked();

private:
    // Setup methods
    void setupUI();
    void groupCellsByFile();
    void createFileTab(const QString& filePath, const QVector<int>& cellIndices);
    void updateCellInfoPanel();
    void updateCellList();
    void updatePreviewImage();

    // Helper methods
    void selectCell(int globalCellIndex);
    void updateRecalcButtonState();
    void recalculateDiameters();
    void loadSavedCoefficient();
    cv::Mat loadImageSafely(const QString& imagePath);
    void saveDebugImage(const QString& originalImagePath,
                       const QVector<QPair<Cell, double>>& cells,
                       const QString& outputPath);

    // UI Elements - Top level
    QTabWidget* m_fileTabWidget;
    QSplitter* m_mainSplitter;

    // Left panel (cell list)
    QScrollArea* m_cellListScrollArea;
    QWidget* m_cellListContainer;
    QVBoxLayout* m_cellListLayout;

    // Right panel (preview + info)
    MarkupImageWidget* m_previewWidget;
    QWidget* m_infoPanel;
    QLabel* m_cellNumberLabel;
    QLabel* m_cellPositionLabel;
    QLabel* m_cellRadiusLabel;
    QLabel* m_cellAreaLabel;

    // Bottom toolbar
    QLineEdit* m_coefficientEdit;
    QPushButton* m_recalcButton;
    QPushButton* m_clearDiametersButton;
    QPushButton* m_statisticsButton;
    QPushButton* m_saveButton;
    QPushButton* m_finishButton;

    // Data
    QVector<Cell> m_cells;
    QMap<QString, QVector<int>> m_cellsByFile; // filepath -> cell indices
    QVector<CellListItemWidget*> m_cellWidgets;
    int m_selectedCellIndex;
    QString m_currentFilePath;
};

#endif // VERIFICATIONWIDGET_H
