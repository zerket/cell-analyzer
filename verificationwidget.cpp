// verificationwidget.cpp
#include "verificationwidget.h"
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QDir>
#include <QStandardPaths>

VerificationWidget::VerificationWidget(const QVector<Cell>& cells, QWidget *parent)
    : QWidget(parent), m_cells(cells)
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    listWidget = new QListWidget(this);
    listWidget->setResizeMode(QListWidget::Adjust);
    listWidget->setViewMode(QListView::IconMode);
    listWidget->setMovement(QListView::Static);
    listWidget->setSpacing(10);
    listWidget->setWrapping(true);
    listWidget->setUniformItemSizes(true);
    listWidget->setWordWrap(true);
    listWidget->setSelectionMode(QAbstractItemView::NoSelection);

    for (const Cell& cell : m_cells) {
        QListWidgetItem* item = new QListWidgetItem(listWidget);
        CellItemWidget* cellWidget = new CellItemWidget(cell);

        item->setSizeHint(cellWidget->sizeHint());

        listWidget->addItem(item);
        listWidget->setItemWidget(item, cellWidget);

        connect(cellWidget, &CellItemWidget::diameterNmChanged, this, &VerificationWidget::onDiameterNmChanged);
        connect(cellWidget, &CellItemWidget::excludedChanged, this, &VerificationWidget::onDiameterNmChanged);
        connect(cellWidget, &CellItemWidget::removeRequested, this, &VerificationWidget::onRemoveCellRequested);
    }

    mainLayout->addWidget(listWidget);

    QHBoxLayout* buttonsLayout = new QHBoxLayout;

    recalcButton = new QPushButton("Пересчитать", this);
    connect(recalcButton, &QPushButton::clicked, this, &VerificationWidget::onRecalculateClicked);
    buttonsLayout->addWidget(recalcButton);

    finishButton = new QPushButton("Завершить", this);
    connect(finishButton, &QPushButton::clicked, this, &VerificationWidget::analysisCompleted);
    buttonsLayout->addWidget(finishButton);

    saveButton = new QPushButton("Сохранить клетки", this);
    connect(saveButton, &QPushButton::clicked, this, &VerificationWidget::onSaveCellsClicked);
    buttonsLayout->addWidget(saveButton);

    clearDiametersButton = new QPushButton("Очистить", this);
    connect(clearDiametersButton, &QPushButton::clicked, this, &VerificationWidget::onClearDiametersClicked);
    buttonsLayout->addWidget(clearDiametersButton);

    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);
}

void VerificationWidget::onDiameterNmChanged() {
    updateRecalcButtonState();
}

void VerificationWidget::updateRecalcButtonState() {
    bool anyFilled = false;
    for (int i = 0; i < listWidget->count(); ++i) {
        CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
        if (w && !w->diameterNmText().isEmpty()) {
            anyFilled = true;
            break;
        }
    }
    //recalcButton->setEnabled(anyFilled);  // <- эта строка закомментирована
}

void VerificationWidget::onRecalculateClicked() {
    recalculateDiameters();
}

void VerificationWidget::recalculateDiameters() {
    QVector<double> scales;

    for (int i = 0; i < listWidget->count(); ++i) {
        CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
        if (!w) continue;

        QString nmText = w->diameterNmText();
        if (!nmText.isEmpty()) {
            double nm = nmText.toDouble();
            int px = w->diameterPx();
            if (px > 0) {
                scales.append(nm / px);
            }
        }
    }

    if (scales.isEmpty()) return;

    double avgScale = 0;
    for (double s : scales) avgScale += s;
    avgScale /= scales.size();

    for (int i = 0; i < listWidget->count(); ++i) {
        CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
        if (!w) continue;

        if (w->diameterNmText().isEmpty()) {
            double nmValue = w->diameterPx() * avgScale;
            w->setDiameterNm(nmValue);
        }
    }
}

void VerificationWidget::onRemoveCellRequested(CellItemWidget* item) {
    // Найти QListWidgetItem, связанный с этим виджетом
    for (int i = 0; i < listWidget->count(); ++i) {
        QListWidgetItem* listItem = listWidget->item(i);
        QWidget* widget = listWidget->itemWidget(listItem);
        if (widget == item) {
            // Удаляем из QListWidget
            delete listWidget->takeItem(i);

            // Удаляем из массива клеток
            if (i >= 0 && i < m_cells.size()) {
                m_cells.removeAt(i);
            }

            // Обновляем состояние кнопок и т.п.
            updateRecalcButtonState();
            break;
        }
    }
}

void VerificationWidget::onSaveCellsClicked() {
    QString outputDir = QDir::currentPath() + "/cells_output";
    QDir dir(outputDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    for (int i = 0; i < listWidget->count(); ++i) {
        CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
        if (!w) continue;

        // Получаем изображение клетки из CellItemWidget (нужно добавить метод getImage() в CellItemWidget)
        QImage img = w->getImage();

        QString filename = dir.filePath(QString("cell_%1.png").arg(i + 1));
        if (!img.save(filename, "PNG")) {
            qWarning() << "Не удалось сохранить файл:" << filename;
        }
    }
}

void VerificationWidget::onClearDiametersClicked() {
    for (int i = 0; i < listWidget->count(); ++i) {
        CellItemWidget* w = qobject_cast<CellItemWidget*>(listWidget->itemWidget(listWidget->item(i)));
        if (w) {
            w->setDiameterNm(.0);
        }
    }
    updateRecalcButtonState();
}
