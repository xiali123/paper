#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GapFinder2Entry {
    int id; QString domain; QString category; QString type;
    qreal severity; int gaps; bool critical; QColor color;
};
class PaperGapFinder2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperGapFinder2(QWidget* parent = nullptr);
    void addEntry(const GapFinder2Entry& entry);
    QList<GapFinder2Entry> entries() const;
    int criticalCount() const;
    qreal avgSeverity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void gapFound(int id, qreal severity);
private slots:
    void onFind();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFinderView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GapFinder2Entry> entries_;
    QSettings settings_;
    QPushButton* findBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
