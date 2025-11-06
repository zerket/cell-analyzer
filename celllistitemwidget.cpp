// celllistitemwidget.cpp
#include "celllistitemwidget.h"
#include <QMouseEvent>
#include <QImage>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include "logger.h"

CellListItemWidget::CellListItemWidget(int cellNumber, const Cell& cell, QWidget* parent)
    : QWidget(parent)
    , m_cellNumber(cellNumber)
    , m_cell(cell)
    , m_selected(false)
    , m_hovered(false)
{
    setupUI();
    setMouseTracking(true);
}

void CellListItemWidget::setupUI()
{
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(8);

    // Cell number
    m_numberLabel = new QLabel(QString("#%1").arg(m_cellNumber));
    m_numberLabel->setFixedWidth(35);
    m_numberLabel->setAlignment(Qt::AlignCenter);
    QFont numberFont = m_numberLabel->font();
    numberFont.setBold(true);
    m_numberLabel->setFont(numberFont);
    layout->addWidget(m_numberLabel);

    // Cell image thumbnail
    m_imageLabel = new QLabel();
    m_imageLabel->setFixedSize(50, 50);
    m_imageLabel->setScaledContents(true);

    if (!m_cell.image.empty()) {
        QImage img = matToQImage(m_cell.image);
        if (!img.isNull()) {
            m_imageLabel->setPixmap(QPixmap::fromImage(img).scaled(50, 50, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
    layout->addWidget(m_imageLabel);

    // Diameter in pixels
    m_diameterPxLabel = new QLabel(QString("%1px").arg(static_cast<int>(m_cell.diameterPx)));
    m_diameterPxLabel->setFixedWidth(50);
    m_diameterPxLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(m_diameterPxLabel);

    // Separator
    QLabel* separator = new QLabel("|");
    separator->setStyleSheet("QLabel { color: #ccc; }");
    layout->addWidget(separator);

    // Diameter in micrometers
    m_diameterNmEdit = new QLineEdit();
    m_diameterNmEdit->setPlaceholderText("мкм");
    m_diameterNmEdit->setFixedWidth(60);
    m_diameterNmEdit->setAlignment(Qt::AlignCenter);
    connect(m_diameterNmEdit, &QLineEdit::textChanged, this, &CellListItemWidget::diameterNmChanged);
    layout->addWidget(m_diameterNmEdit);

    // Remove button
    m_removeButton = new QPushButton("❌");
    m_removeButton->setFixedSize(30, 30);
    m_removeButton->setStyleSheet("QPushButton { border: none; background: transparent; font-size: 14px; } QPushButton:hover { background-color: #ffebee; border-radius: 5px; }");
    connect(m_removeButton, &QPushButton::clicked, [this]() {
        emit removeRequested(this);
    });
    layout->addWidget(m_removeButton);

    setLayout(layout);
    updateStyle();
}

QString CellListItemWidget::diameterNmText() const
{
    return m_diameterNmEdit->text();
}

double CellListItemWidget::getDiameterNm() const
{
    QString text = m_diameterNmEdit->text();
    if (text.isEmpty()) return 0.0;

    bool ok;
    double value = text.toDouble(&ok);
    return ok ? value : 0.0;
}

void CellListItemWidget::setDiameterNm(double nm)
{
    if (nm > 0) {
        m_diameterNmEdit->setText(QString::number(nm, 'f', 2));
    } else {
        m_diameterNmEdit->clear();
    }
}

void CellListItemWidget::clearDiameterNm()
{
    m_diameterNmEdit->clear();
}

void CellListItemWidget::setSelected(bool selected)
{
    m_selected = selected;
    updateStyle();
}

void CellListItemWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(this);
    }
    QWidget::mousePressEvent(event);
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
void CellListItemWidget::enterEvent(QEnterEvent* event)
#else
void CellListItemWidget::enterEvent(QEvent* event)
#endif
{
    m_hovered = true;
    updateStyle();
    QWidget::enterEvent(event);
}

void CellListItemWidget::leaveEvent(QEvent* event)
{
    m_hovered = false;
    updateStyle();
    QWidget::leaveEvent(event);
}

void CellListItemWidget::updateStyle()
{
    QString style;

    if (m_selected) {
        // Серая подсветка для выделенной клетки, текст не меняется
        style = "CellListItemWidget { "
                "background-color: #BDBDBD; "  // Серый фон
                "border: 2px solid #9E9E9E; "   // Темно-серая рамка
                "border-radius: 5px; "
                "}";
    } else if (m_hovered) {
        // Светлая подсветка при наведении
        style = "CellListItemWidget { "
                "background-color: #E3F2FD; "
                "border: 2px solid #90CAF9; "
                "border-radius: 5px; "
                "}";
    } else {
        // Обычное состояние
        style = "CellListItemWidget { "
                "background-color: white; "
                "border: 1px solid #E0E0E0; "
                "border-radius: 5px; "
                "}";
    }

    setStyleSheet(style);
}
