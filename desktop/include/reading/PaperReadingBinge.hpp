#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BingeEntry {
    int id; QString session; QString category; QString mode;
    int papers; qreal hours; qreal pages; qreal score; bool marathon; QColor color;
};
class PaperReadingBinge : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingBinge(QWidget* parent = nullptr);
    void addEntry(const BingeEntry& entry);
    QList<BingeEntry> entries() const;
    int marathonCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bingeRecorded(int id, qreal score);
private slots:
    void onRecord();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBingeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BingeEntry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
