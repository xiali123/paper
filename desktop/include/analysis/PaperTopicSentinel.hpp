#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SentinelEntry {
    int id; QString topic; QString category; QString alert;
    qreal trend; int mentions; bool rising; QColor color;
};
class PaperTopicSentinel : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicSentinel(QWidget* parent = nullptr);
    void addEntry(const SentinelEntry& entry);
    QList<SentinelEntry> entries() const;
    int risingCount() const;
    qreal avgTrend() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void topicAlerted(int id, qreal trend);
private slots:
    void onMonitor();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSentinelView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SentinelEntry> entries_;
    QSettings settings_;
    QPushButton* monitorBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
