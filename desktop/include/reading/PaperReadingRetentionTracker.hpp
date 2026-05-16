#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

struct RetentionEntry {
    int id;
    QString paperTitle;
    int day;
    qreal retention;
    QString method;
    int reviewCount;
    qreal confidence;
    QString topic;
    QColor color;
};

class PaperReadingRetentionTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingRetentionTracker(QWidget* parent = nullptr);
    void addEntry(const RetentionEntry& entry);
    QList<RetentionEntry> entries() const;
    qreal avgRetention() const;
    int totalReviews() const;
    QMap<QString, int> methodCounts() const;

signals:
    void retentionRecorded(int id, qreal retention);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawRetentionList(QPainter& p, const QRect& rect);
    void drawDecayChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<RetentionEntry> entries_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
