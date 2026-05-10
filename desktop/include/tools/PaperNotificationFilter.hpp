#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct NotificationRule {
    int id;
    QString ruleName;
    QString channel;
    QString priority;
    QString keyword;
    QString action;
    bool active;
    bool regex;
    QColor color;
};

class PaperNotificationFilter : public QWidget {
    Q_OBJECT
public:
    explicit PaperNotificationFilter(QWidget* parent = nullptr);
    void addEntry(const NotificationRule& entry);
    QList<NotificationRule> entries() const;
    int activeCount() const;
    int regexCount() const;
    QMap<QString, int> channelCounts() const;

signals:
    void filterCreated(int id, QString ruleName);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawRuleList(QPainter& p, const QRect& rect);
    void drawChannelChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* channelCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<NotificationRule> entries_;
};
