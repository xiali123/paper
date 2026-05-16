#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QList>
#include <QSettings>

struct QaMessage {
    int id{-1};
    QString text;
    QString sender; // "user" or "assistant"
    QTime time;
    QString paperTitle;
    int paperId{-1};
};

class PaperQaChatWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperQaChatWidget(QWidget* parent = nullptr);

    void addMessage(const QaMessage& msg);
    QList<QaMessage> messages() const;
    int messageCount() const;
    void setPaper(const QString& title, int id);

signals:
    void questionAsked(const QString& question, int paperId);
    void answerReady(const QString& answer);

private slots:
    void onSend();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawChat(QPainter& p, const QRect& rect);
    void drawInput(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_{nullptr};
    QPushButton* sendBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<QaMessage> messages_;
    QString paperTitle_;
    int paperId_{-1};
    QSettings settings_;
};
