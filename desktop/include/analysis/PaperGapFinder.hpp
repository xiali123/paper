#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GapEntry {
    int id; QString topic; QString category; QString gap;
    qreal importance; int references; bool addressed; QColor color;
};
class PaperGapFinder : public QWidget {
    Q_OBJECT
public:
    explicit PaperGapFinder(QWidget* parent = nullptr);
    void addEntry(const GapEntry& entry);
    QList<GapEntry> entries() const;
    int addressedCount() const;
    qreal avgImportance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void gapIdentified(int id, qreal importance);
private slots:
    void onFind();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGapMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GapEntry> entries_;
    QSettings settings_;
    QPushButton* findBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
