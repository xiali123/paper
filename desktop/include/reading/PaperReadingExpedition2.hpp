#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingExpedition2Entry {
    int id; QString title; QString category; QString terrain;
    qreal progress; int milestones; bool completed; QColor color;
};
class PaperReadingExpedition2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingExpedition2(QWidget* parent = nullptr);
    void addEntry(const ReadingExpedition2Entry& entry);
    QList<ReadingExpedition2Entry> entries() const;
    int completedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void expeditionCompleted(int id, qreal progress);
private slots:
    void onStart();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawExpeditionView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingExpedition2Entry> entries_;
    QSettings settings_;
    QPushButton* startBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
