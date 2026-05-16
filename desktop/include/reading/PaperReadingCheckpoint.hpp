#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CheckpointEntry {
    int id; QString paper; QString category; QString status;
    qreal progress; int pagesRead; bool passed; QColor color;
};
class PaperReadingCheckpoint : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingCheckpoint(QWidget* parent = nullptr);
    void addEntry(const CheckpointEntry& entry);
    QList<CheckpointEntry> entries() const;
    int passedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void checkpointReached(int id, qreal progress);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCheckpointTrack(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CheckpointEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
