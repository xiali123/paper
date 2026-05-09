#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>

struct SortEntry {
    int id;
    QString paperTitle;
    QString category;
    int priority;
    qreal relevance;
    int pageCount;
    QString difficulty;
    QString status;
    QColor color;
};

class PaperReadingListSorter : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingListSorter(QWidget* parent = nullptr);
    void addEntry(const SortEntry& entry);
    QList<SortEntry> entries() const;
    QMap<QString, int> difficultyCounts() const;
    qreal avgRelevance() const;
    int totalPages() const;

signals:
    void listSorted(int count);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onSort();
    void onClear();
    void drawSortedList(QPainter& p, const QRect& rect);
    void drawDifficultyChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<SortEntry> entries_;
    QPushButton* addBtn_;
    QPushButton* sortBtn_;
    QPushButton* clearBtn_;
    QComboBox* sortCombo_;
    QLabel* infoLabel_;
};
