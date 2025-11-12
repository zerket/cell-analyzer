// celllistitemwidget.cpp
#include "celllistitemwidget.h"
#include <QMouseEvent>
#include <QImage>
#include <QPalette>
#include <opencv2/opencv.hpp>
#include "utils.h"
#include "logger.h"

CellListItemWidget::CellListItemWidget(int cellNumber, const Cell& cell, QWidget* parent)
    : QWidget(parent)
    , m_cellNumber(cellNumber)
    , m_cell(cell)
    , m_selected(false)
    , m_hovered(false)
    , m_thumbnailLoaded(false)
{
    setAutoFillBackground(true);  // Enable background painting
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
    m_imageLabel->setScaledContents(false); // Изменено на false для лучшей производительности
    m_imageLabel->setAlignment(Qt::AlignCenter);

    // Оптимизация: используем заполнитель вместо реального изображения для экономии памяти
    // Реальное изображение будет загружено только при необходимости
    m_imageLabel->setText("📷");
    m_imageLabel->setStyleSheet("QLabel { background-color: #f0f0f0; font-size: 20px; }");

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
    if (m_selected != selected) {
        m_selected = selected;
        updateStyle();

        LOG_DEBUG(QString("Cell #%1 selection changed to: %2").arg(m_cellNumber).arg(selected ? "true" : "false"));

        // Загружаем thumbnail только для выделенной ячейки
        if (selected && !m_thumbnailLoaded) {
            loadThumbnail();
        }
    }
}

void CellListItemWidget::loadThumbnail()
{
    if (m_thumbnailLoaded || m_cell.image.empty()) {
        return;
    }

    QImage img = matToQImage(m_cell.image);
    if (!img.isNull()) {
        // Используем FastTransformation вместо SmoothTransformation для скорости
        m_imageLabel->setPixmap(QPixmap::fromImage(img).scaled(50, 50, Qt::KeepAspectRatio, Qt::FastTransformation));
        m_imageLabel->setStyleSheet(""); // Убираем стиль заполнителя
        m_thumbnailLoaded = true;
    }
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
        // Яркая синяя подсветка для выделенной клетки
        style = "QWidget { "
                "background-color: #2196F3; "  // Яркий синий фон
                "border: 3px solid #1976D2; "   // Темно-синяя толстая рамка
                "border-radius: 5px; "
                "}"
                "QLabel { color: white; font-weight: bold; }"  // Белый жирный текст
                "QLineEdit { background-color: white; color: black; border: 1px solid #1976D2; }";  // Белое поле ввода

        LOG_DEBUG(QString("Applying SELECTED style to cell #%1").arg(m_cellNumber));

        // Also set palette for background
        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(33, 150, 243));  // #2196F3
        setPalette(pal);
    } else if (m_hovered) {
        // Светлая подсветка при наведении
        style = "QWidget { "
                "background-color: #E3F2FD; "
                "border: 2px solid #90CAF9; "
                "border-radius: 5px; "
                "}";

        QPalette pal = palette();
        pal.setColor(QPalette::Window, QColor(227, 242, 253));  // #E3F2FD
        setPalette(pal);
    } else {
        // Обычное состояние
        style = "QWidget { "
                "background-color: white; "
                "border: 1px solid #E0E0E0; "
                "border-radius: 5px; "
                "}";

        QPalette pal = palette();
        pal.setColor(QPalette::Window, Qt::white);
        setPalette(pal);
    }

    setStyleSheet(style);

    // Force immediate repaint
    update();
    repaint();
}
