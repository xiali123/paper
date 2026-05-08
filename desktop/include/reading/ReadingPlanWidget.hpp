#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct PlanEntry {
    int id{-1};
    QString title;
    QString author;
    int totalPages{0};
    int pagesRead{0};
    QDate startDate;
    QDate deadline;
    int priority{0};
    QString status; // "not_started", "reading", "completed"
    QString notes;
};

class ReadingPlanWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReadingPlanWidget(QWidget* parent = nullptr);

    void addEntry(const PlanEntry& entry);
    QList<PlanEntry> entries() const;
    qreal completionRate() const;
    int overdueCount() const;
    int totalPagesRead() const;

signals:
    void entryClicked(int entryId);
    void planUpdated(int completed, int total);

private slots:
    void onAdd();
    void onRemove();
    void onSortChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTimeline(QPainter& p, const QRect& rect);
    void drawProgressBars(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* sortCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<PlanEntry> entries_;
    int selectedEntry_{-1};
    QSettings settings_;
};
