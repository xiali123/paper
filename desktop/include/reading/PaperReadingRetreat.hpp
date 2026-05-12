#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RetreatEntry {
    int id; QString topic; QString category; QString focus;
    qreal depth; int hours; bool completed; QColor color;
};
class PaperReadingRetreat : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingRetreat(QWidget* parent = nullptr);
    void addEntry(const RetreatEntry& entry);
    QList<RetreatEntry> entries() const;
    int completedCount() const;
    qreal avgDepth() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void retreatDone(int id, qreal depth);
private slots:
    void onStart();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRetreatTimeline(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RetreatEntry> entries_;
    QSettings settings_;
    QPushButton* startBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
