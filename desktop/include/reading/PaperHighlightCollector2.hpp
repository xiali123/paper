#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HighlightCollector2Entry {
    int id; QString text; QString category; QString color;
    qreal relevance; int highlights; bool starred; QColor entryColor;
};
class PaperHighlightCollector2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperHighlightCollector2(QWidget* parent = nullptr);
    void addEntry(const HighlightCollector2Entry& entry);
    QList<HighlightCollector2Entry> entries() const;
    int starredCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void highlightCollected(int id, qreal relevance);
private slots:
    void onCollect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCollectorView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HighlightCollector2Entry> entries_;
    QSettings settings_;
    QPushButton* collectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
