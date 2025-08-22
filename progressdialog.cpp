#include "progressdialog.h"
#include <QTextEdit>
#include <QScrollArea>
#include <QApplication>
#include <QScreen>

ProgressDialog::ProgressDialog(QWidget* parent)
    : QDialog(parent)
    , m_canceled(false)
    , m_estimatedTimeSeconds(0)
{
    setupUI();
    
    // Создаем таймер для обновления времени
    m_timeUpdateTimer = new QTimer(this);
    connect(m_timeUpdateTimer, &QTimer::timeout, this, &ProgressDialog::updateElapsedTime);
    
    setModal(true);
    setWindowFlags(Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint);
    resize(400, 200);
    
    // Центрируем диалог
    if (parent) {
        move(parent->geometry().center() - rect().center());
    } else {
        QScreen* screen = QApplication::primaryScreen();
        move(screen->geometry().center() - rect().center());
    }
}

ProgressDialog::~ProgressDialog() {
    if (m_loadingAnimation) {
        m_loadingAnimation->stop();
        delete m_loadingAnimation;
    }
}

void ProgressDialog::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    
    // Заголовок с анимацией
    QHBoxLayout* headerLayout = new QHBoxLayout();
    
    m_animationLabel = new QLabel();
    m_animationLabel->setFixedSize(32, 32);
    m_animationLabel->setStyleSheet("QLabel { background: transparent; }");
    
    // Создаем простую анимацию загрузки
    m_loadingAnimation = new QMovie();
    // Создаем простую анимацию программно (заменяем на GIF если есть)
    m_animationLabel->setText("⏳");
    m_animationLabel->setAlignment(Qt::AlignCenter);
    
    headerLayout->addWidget(m_animationLabel);
    headerLayout->addStretch();
    
    mainLayout->addLayout(headerLayout);
    
    // Сообщение
    m_messageLabel = new QLabel("Выполняется операция...");
    m_messageLabel->setWordWrap(true);
    m_messageLabel->setStyleSheet("QLabel { font-size: 14px; color: #333; }");
    m_messageLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_messageLabel);
    
    // Прогресс-бар
    m_progressBar = new QProgressBar();
    m_progressBar->setStyleSheet(
        "QProgressBar {"
        "    border: 2px solid #ddd;"
        "    border-radius: 8px;"
        "    text-align: center;"
        "    font-weight: bold;"
        "    background-color: #f0f0f0;"
        "}"
        "QProgressBar::chunk {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, stop:0 #4CAF50, stop:1 #45a049);"
        "    border-radius: 6px;"
        "}"
    );
    m_progressBar->setMinimumHeight(25);
    mainLayout->addWidget(m_progressBar);
    
    // Информация о времени
    m_timeLabel = new QLabel();
    m_timeLabel->setStyleSheet("QLabel { font-size: 12px; color: #666; }");
    m_timeLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_timeLabel);
    
    // Лог сообщений (скрытый по умолчанию)
    m_logLabel = new QLabel();
    m_logLabel->setStyleSheet(
        "QLabel {"
        "    background-color: #f9f9f9;"
        "    border: 1px solid #ddd;"
        "    border-radius: 4px;"
        "    padding: 8px;"
        "    font-family: monospace;"
        "    font-size: 10px;"
        "}"
    );
    m_logLabel->setWordWrap(true);
    m_logLabel->setMaximumHeight(80);
    m_logLabel->hide(); // Скрываем по умолчанию
    mainLayout->addWidget(m_logLabel);
    
    // Кнопка отмены
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_cancelButton = new QPushButton("Отмена");
    m_cancelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #f44336;"
        "    color: white;"
        "    border: none;"
        "    border-radius: 6px;"
        "    padding: 8px 16px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #d32f2f;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #b71c1c;"
        "}"
    );
    connect(m_cancelButton, &QPushButton::clicked, this, &ProgressDialog::onCancelClicked);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addStretch();
    
    mainLayout->addLayout(buttonLayout);
    
    setLayout(mainLayout);
}

void ProgressDialog::setTitle(const QString& title) {
    setWindowTitle(title);
}

void ProgressDialog::setMessage(const QString& message) {
    m_messageLabel->setText(message);
}

void ProgressDialog::setProgress(int value) {
    m_progressBar->setValue(value);
    
    // Обновляем текст прогресс-бара
    if (m_progressBar->maximum() > 0) {
        int percentage = (value * 100) / m_progressBar->maximum();
        m_progressBar->setFormat(QString("%1% (%2 из %3)").arg(percentage).arg(value).arg(m_progressBar->maximum()));
    }
    
    // Обновляем оценку времени
    updateTimeDisplay();
}

void ProgressDialog::setMaximum(int maximum) {
    m_progressBar->setMaximum(maximum);
}

void ProgressDialog::setRange(int minimum, int maximum) {
    m_progressBar->setRange(minimum, maximum);
}

void ProgressDialog::showIndeterminate(const QString& message) {
    setMessage(message);
    m_progressBar->setRange(0, 0); // Неопределенный прогресс
    m_progressBar->setFormat("Выполняется...");
    
    m_elapsedTimer.start();
    m_timeUpdateTimer->start(1000); // Обновляем каждую секунду
    
    show();
    QApplication::processEvents();
}

void ProgressDialog::showDeterminate(const QString& message, int maximum) {
    setMessage(message);
    setRange(0, maximum);
    setProgress(0);
    
    m_elapsedTimer.start();
    m_timeUpdateTimer->start(1000);
    
    show();
    QApplication::processEvents();
}

void ProgressDialog::addLogMessage(const QString& message) {
    if (!m_logMessages.isEmpty()) {
        m_logMessages += "\n";
    }
    m_logMessages += message;
    
    // Ограничиваем количество сообщений
    QStringList lines = m_logMessages.split('\n');
    if (lines.size() > 5) {
        lines = lines.mid(lines.size() - 5);
        m_logMessages = lines.join('\n');
    }
    
    m_logLabel->setText(m_logMessages);
    
    if (m_logLabel->isHidden()) {
        m_logLabel->show();
        resize(width(), sizeHint().height());
    }
    
    QApplication::processEvents();
}

void ProgressDialog::setEstimatedTime(int seconds) {
    m_estimatedTimeSeconds = seconds;
    updateTimeDisplay();
}

void ProgressDialog::reset() {
    m_canceled = false;
    setProgress(0);
    m_logMessages.clear();
    m_logLabel->clear();
    m_logLabel->hide();
    m_timeUpdateTimer->stop();
}

void ProgressDialog::close() {
    m_timeUpdateTimer->stop();
    QDialog::close();
}

void ProgressDialog::onCancelClicked() {
    m_canceled = true;
    m_cancelButton->setEnabled(false);
    m_cancelButton->setText("Отменяется...");
    
    emit canceled();
}

void ProgressDialog::updateElapsedTime() {
    updateTimeDisplay();
}

void ProgressDialog::updateTimeDisplay() {
    if (!m_elapsedTimer.isValid()) return;
    
    qint64 elapsed = m_elapsedTimer.elapsed() / 1000; // в секундах
    
    QString timeText = QString("Прошло времени: %1").arg(formatTime(elapsed));
    
    // Добавляем оценку оставшегося времени для определенного прогресса
    if (m_progressBar->maximum() > 0 && m_progressBar->value() > 0) {
        int progress = m_progressBar->value();
        int total = m_progressBar->maximum();
        
        if (progress > 0) {
            qint64 estimatedTotal = (elapsed * total) / progress;
            qint64 remaining = estimatedTotal - elapsed;
            
            if (remaining > 0) {
                timeText += QString(" | Осталось: %1").arg(formatTime(remaining));
            }
        }
    } else if (m_estimatedTimeSeconds > 0) {
        timeText += QString(" | Ожидается: %1").arg(formatTime(m_estimatedTimeSeconds));
    }
    
    m_timeLabel->setText(timeText);
}

QString ProgressDialog::formatTime(qint64 seconds) {
    if (seconds < 60) {
        return QString("%1 сек").arg(seconds);
    } else if (seconds < 3600) {
        int mins = seconds / 60;
        int secs = seconds % 60;
        return QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));
    } else {
        int hours = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        return QString("%1 ч %2 мин").arg(hours).arg(mins);
    }
}