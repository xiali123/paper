#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct MeetingNoteEntry {
    int id; QString title; QString category; QString attendees;
    qreal duration; int actionItems; bool followUp; QColor color;
};
class PaperMeetingNotes : public QWidget {
    Q_OBJECT
public:
    explicit PaperMeetingNotes(QWidget* parent = nullptr);
    void addEntry(const MeetingNoteEntry& entry);
    QList<MeetingNoteEntry> entries() const;
    int followUpCount() const;
    qreal avgDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void noteSaved(int id, qreal duration);
private slots:
    void onSave();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawNoteList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MeetingNoteEntry> entries_;
    QSettings settings_;
    QPushButton* saveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
