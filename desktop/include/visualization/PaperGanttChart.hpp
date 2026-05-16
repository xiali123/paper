#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct GanttEntry {
    int id;
    QString taskName;
    QString category;
    int start;
    int duration;
    int progress;
    QString dependency;
    bool milestone;
    QColor color;
};

class PaperGanttChart : public QWidget {
    Q_OBJECT
public:
    explicit PaperGanttChart(QWidget* parent = nullptr);
    void addEntry(const GanttEntry& entry);
    QList<GanttEntry> entries() const;
    int milestoneCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void ganttCreated(int id, int progress);

private slots:
    void onGenerate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawGanttView(QPainter& p, const QRect& rect);
    void drawCategoryLegend(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<GanttEntry> entries_;
    QSettings settings_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
