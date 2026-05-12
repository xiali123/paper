#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TopicClusterEntry {
    int id; QString topic; QString category; QString cluster;
    qreal similarity; int papers; bool core; QColor color;
};
class PaperTopicClusterer2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicClusterer2(QWidget* parent = nullptr);
    void addEntry(const TopicClusterEntry& entry);
    QList<TopicClusterEntry> entries() const;
    int coreCount() const;
    qreal avgSimilarity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void clusterFound(int id, qreal similarity);
private slots:
    void onCluster();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawClusterView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TopicClusterEntry> entries_;
    QSettings settings_;
    QPushButton* clusterBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
