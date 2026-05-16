#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RateLimiter2Entry {
    int id; QString endpoint; QString category; QString method;
    qreal rate; int requests; bool throttled; QColor color;
};
class PaperRateLimiter2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperRateLimiter2(QWidget* parent = nullptr);
    void addEntry(const RateLimiter2Entry& entry);
    QList<RateLimiter2Entry> entries() const;
    int throttledCount() const;
    qreal avgRate() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void rateChecked(int id, qreal rate);
private slots:
    void onCheck();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLimiterView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RateLimiter2Entry> entries_;
    QSettings settings_;
    QPushButton* checkBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
