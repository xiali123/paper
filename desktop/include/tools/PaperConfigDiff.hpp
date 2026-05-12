#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DiffEntry {
    int id; QString file; QString category; QString change;
    qreal impact; int lines; bool breaking; QColor color;
};
class PaperConfigDiff : public QWidget {
    Q_OBJECT
public:
    explicit PaperConfigDiff(QWidget* parent = nullptr);
    void addEntry(const DiffEntry& entry);
    QList<DiffEntry> entries() const;
    int breakingCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void diffFound(int id, qreal impact);
private slots:
    void onCompare();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDiffView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DiffEntry> entries_;
    QSettings settings_;
    QPushButton* compareBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
