#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ExchangeEntry {
    int id; QString paper; QString category; QString reviewer;
    qreal quality; int rounds; bool consensus; QColor color;
};
class PaperReviewExchange : public QWidget {
    Q_OBJECT
public:
    explicit PaperReviewExchange(QWidget* parent = nullptr);
    void addEntry(const ExchangeEntry& entry);
    QList<ExchangeEntry> entries() const;
    int consensusCount() const;
    qreal avgQuality() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void reviewDone(int id, qreal quality);
private slots:
    void onExchange();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawExchangeBoard(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ExchangeEntry> entries_;
    QSettings settings_;
    QPushButton* exchangeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
