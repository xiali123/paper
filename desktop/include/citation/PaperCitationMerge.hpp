#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationMergeEntry {
    int id; QString left; QString category; QString right;
    qreal similarity; int conflicts; bool merged; QColor color;
};
class PaperCitationMerge : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationMerge(QWidget* parent = nullptr);
    void addEntry(const CitationMergeEntry& entry);
    QList<CitationMergeEntry> entries() const;
    int mergedCount() const;
    qreal avgSimilarity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void citationMerged(int id, qreal similarity);
private slots:
    void onMerge();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMergeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationMergeEntry> entries_;
    QSettings settings_;
    QPushButton* mergeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
