#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingOdyssey2Entry {
    int id; QString chapter; QString category; QString quest;
    qreal progress; int pages; bool epic; QColor color;
};
class PaperReadingOdyssey2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingOdyssey2(QWidget* parent = nullptr);
    void addEntry(const ReadingOdyssey2Entry& entry);
    QList<ReadingOdyssey2Entry> entries() const;
    int epicCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void questCompleted(int id, qreal progress);
private slots:
    void onExplore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawOdysseyView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingOdyssey2Entry> entries_;
    QSettings settings_;
    QPushButton* exploreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
