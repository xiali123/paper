#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct MarathonEntry {
    int id; QString session; QString category; QString phase;
    qreal pace; int pages; bool finished; QColor color;
};
class PaperReadingMarathon : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingMarathon(QWidget* parent = nullptr);
    void addEntry(const MarathonEntry& entry);
    QList<MarathonEntry> entries() const;
    int finishedCount() const;
    qreal avgPace() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void marathonDone(int id, qreal pace);
private slots:
    void onStart();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMarathonTrack(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MarathonEntry> entries_;
    QSettings settings_;
    QPushButton* startBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
