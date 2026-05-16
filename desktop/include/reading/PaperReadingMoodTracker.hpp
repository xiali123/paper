#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

struct MoodEntry {
    int id;
    QString paperTitle;
    QString mood;
    int rating;
    QString session;
    qreal focus;
    int duration;
    QString notes;
    QColor color;
};

class PaperReadingMoodTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingMoodTracker(QWidget* parent = nullptr);
    void addEntry(const MoodEntry& entry);
    QList<MoodEntry> entries() const;
    QMap<QString, int> moodCounts() const;
    qreal avgRating() const;
    qreal avgFocus() const;

signals:
    void moodRecorded(int id, const QString& mood);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawMoodList(QPainter& p, const QRect& rect);
    void drawMoodChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<MoodEntry> entries_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* moodCombo_;
    QLabel* infoLabel_;
};
