#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingDashboard2Entry {
    int id; QString title; QString category; QString status;
    qreal progress; int pages; bool overdue; QColor color;
};
class PaperReadingDashboard2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingDashboard2(QWidget* parent = nullptr);
    void addEntry(const ReadingDashboard2Entry& entry);
    QList<ReadingDashboard2Entry> entries() const;
    int overdueCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void readingUpdated(int id, qreal progress);
private slots:
    void onRefresh();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDashboardView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingDashboard2Entry> entries_;
    QSettings settings_;
    QPushButton* refreshBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
