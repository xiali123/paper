#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RateEntry {
    int id; QString endpoint; QString category; QString tier;
    int limit; int used; qreal utilization; bool throttled; QColor color;
};
class PaperRateLimiter : public QWidget {
    Q_OBJECT
public:
    explicit PaperRateLimiter(QWidget* parent = nullptr);
    void addEntry(const RateEntry& entry);
    QList<RateEntry> entries() const;
    int throttledCount() const;
    qreal avgUtilization() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rateChecked(int id, qreal utilization);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRateList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RateEntry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
