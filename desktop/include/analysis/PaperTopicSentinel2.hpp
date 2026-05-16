#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TopicSentinel2Entry {
    int id; QString topic; QString category; QString trend;
    qreal momentum; int mentions; bool emerging; QColor color;
};
class PaperTopicSentinel2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicSentinel2(QWidget* parent = nullptr);
    void addEntry(const TopicSentinel2Entry& entry);
    QList<TopicSentinel2Entry> entries() const;
    int emergingCount() const;
    qreal avgMomentum() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void topicSpiked(int id, qreal momentum);
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
    QList<TopicSentinel2Entry> entries_;
    QSettings settings_;
    QPushButton* monitorBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
