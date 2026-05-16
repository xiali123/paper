#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ClusterEntry {
    int id; QString topic; QString category; QString cluster;
    qreal cohesion; qreal separation; int papers; bool core; QColor color;
};
class PaperTopicCluster : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicCluster(QWidget* parent = nullptr);
    void addEntry(const ClusterEntry& entry);
    QList<ClusterEntry> entries() const;
    int coreCount() const;
    qreal avgCohesion() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void clusterFormed(int id, qreal cohesion);
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
    QList<ClusterEntry> entries_;
    QSettings settings_;
    QPushButton* clusterBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
