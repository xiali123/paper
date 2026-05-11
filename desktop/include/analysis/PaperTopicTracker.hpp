#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TopicEntry {
    int id; QString topic; QString category; QString trend;
    qreal frequency; qreal growth; int papers; bool rising; QColor color;
};
class PaperTopicTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicTracker(QWidget* parent = nullptr);
    void addEntry(const TopicEntry& entry);
    QList<TopicEntry> entries() const;
    int risingCount() const;
    qreal avgGrowth() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void topicTracked(int id, qreal growth);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTopicList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TopicEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
