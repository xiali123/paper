#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SprintEntry {
    int id; QString paper; QString category; QString phase;
    qreal pace; qreal pages; qreal focus; bool completed; QColor color;
};
class PaperReadingSprint : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingSprint(QWidget* parent = nullptr);
    void addEntry(const SprintEntry& entry);
    QList<SprintEntry> entries() const;
    int completedCount() const;
    qreal avgPace() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sprintRecorded(int id, qreal pace);
private slots:
    void onRecord();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSprintView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SprintEntry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
