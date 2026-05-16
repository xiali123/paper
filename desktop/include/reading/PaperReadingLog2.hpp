#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingLog2Entry {
    int id; QString title; QString category; QString journal;
    qreal pages; int minutes; bool finished; QColor color;
};
class PaperReadingLog2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingLog2(QWidget* parent = nullptr);
    void addEntry(const ReadingLog2Entry& entry);
    QList<ReadingLog2Entry> entries() const;
    int finishedCount() const;
    qreal totalPages() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void logUpdated(int id, qreal pages);
private slots:
    void onLog();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLogView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingLog2Entry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
