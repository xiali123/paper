#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LogAggregator2Entry {
    int id; QString source; QString category; QString level;
    qreal volume; int alerts; bool critical; QColor color;
};
class PaperLogAggregator2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogAggregator2(QWidget* parent = nullptr);
    void addEntry(const LogAggregator2Entry& entry);
    QList<LogAggregator2Entry> entries() const;
    int criticalCount() const;
    qreal avgVolume() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void alertTriggered(int id, qreal volume);
private slots:
    void onAggregate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAggregatorView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogAggregator2Entry> entries_;
    QSettings settings_;
    QPushButton* aggregateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
