#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TopicEvoEntry {
    int id; QString topic; QString category; QString era;
    qreal popularity; int papers; bool trending; QColor color;
};
class PaperTopicEvolution2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicEvolution2(QWidget* parent = nullptr);
    void addEntry(const TopicEvoEntry& entry);
    QList<TopicEvoEntry> entries() const;
    int trendingCount() const;
    qreal avgPopularity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void evolutionTracked(int id, qreal popularity);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawEvolutionChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TopicEvoEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
