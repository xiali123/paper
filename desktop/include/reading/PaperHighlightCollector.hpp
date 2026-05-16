#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HighlightEntry {
    int id; QString text; QString category; QString color;
    qreal relevance; int annotations; bool starred; QColor highlightColor;
};
class PaperHighlightCollector : public QWidget {
    Q_OBJECT
public:
    explicit PaperHighlightCollector(QWidget* parent = nullptr);
    void addEntry(const HighlightEntry& entry);
    QList<HighlightEntry> entries() const;
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
    void drawCollectionView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HighlightEntry> entries_;
    QSettings settings_;
    QPushButton* collectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
