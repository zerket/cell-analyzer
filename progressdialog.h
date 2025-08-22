#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMovie>
#include <QTimer>
#include <QElapsedTimer>

class ProgressDialog : public QDialog {
    Q_OBJECT

public:
    explicit ProgressDialog(QWidget* parent = nullptr);
    ~ProgressDialog();
    
    void setTitle(const QString& title);
    void setMessage(const QString& message);
    void setProgress(int value);
    void setMaximum(int maximum);
    void setRange(int minimum, int maximum);
    
    void showIndeterminate(const QString& message = "Загрузка...");
    void showDeterminate(const QString& message, int maximum);
    
    void addLogMessage(const QString& message);
    void setEstimatedTime(int seconds);
    
    bool wasCanceled() const { return m_canceled; }

public slots:
    void reset();
    void close();

signals:
    void canceled();

private slots:
    void onCancelClicked();
    void updateElapsedTime();

private:
    void setupUI();
    void updateTimeDisplay();
    QString formatTime(qint64 seconds);
    
private:
    QProgressBar* m_progressBar;
    QLabel* m_messageLabel;
    QLabel* m_animationLabel;
    QLabel* m_timeLabel;
    QLabel* m_logLabel;
    QPushButton* m_cancelButton;
    
    QMovie* m_loadingAnimation;
    QTimer* m_timeUpdateTimer;
    QElapsedTimer m_elapsedTimer;
    
    bool m_canceled;
    int m_estimatedTimeSeconds;
    QString m_logMessages;
};

#endif // PROGRESSDIALOG_H