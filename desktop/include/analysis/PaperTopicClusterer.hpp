#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct TopicClusterEntry {
    int id;
    QString topic;
    QString cluster;
    qreal similarity;
    int paperCount;
    QString keywords;
    qreal coherence;
    QString category;
    bool dominant;
    QColor color;
};

class PaperTopicClusterer : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicClusterer(QWidget* parent = nullptr);
    void addEntry(const TopicClusterEntry& entry);
    QList<TopicClusterEntry> entries() const;
    qreal avgSimilarity() const;
    int dominantCount() const;
    QMap<QString, int> clusterCounts() const;

signals:
    void topicClustered(int id, qreal similarity);

private slots:
    void onCluster();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawTopicList(QPainter& p, const QRect& rect);
    void drawClusterChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<TopicClusterEntry> entries_;
    QPushButton* clusterBtn_;
    QPushButton* clearBtn_;
    QComboBox* methodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
