#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct CalendarEntry {
    int id;
    QString title;
    QString date;
    QString category;
    int duration;
    qreal pagesRead;
    QString difficulty;
    bool completed;
    QColor color;
};

class PaperReadingCalendar : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingCalendar(QWidget* parent = nullptr);
    void addEntry(const CalendarEntry& entry);
    QList<CalendarEntry> entries() const;
    int completedCount() const;
    qreal totalPages() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void calendarUpdated(int id, qreal pagesRead);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCalendarGrid(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<CalendarEntry> entries_;
};
