#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct MethodologyCompareEntry {
    int id;
    QString methodA;
    QString methodB;
    QString dimension;
    qreal similarity;
    QString strength;
    QString weakness;
    QString domain;
    int papersUsing;
    QColor color;
};

class PaperMethodologyComparator : public QWidget {
    Q_OBJECT
public:
    explicit PaperMethodologyComparator(QWidget* parent = nullptr);
    void addEntry(const MethodologyCompareEntry& entry);
    QList<MethodologyCompareEntry> entries() const;
    qreal avgSimilarity() const;
    int compatiblePairs() const;
    QMap<QString, int> domainCounts() const;

signals:
    void comparisonDone(int id, qreal similarity);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onCompare();
    void onClear();
    void drawCompareList(QPainter& p, const QRect& rect);
    void drawDomainChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* compareBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<MethodologyCompareEntry> entries_;
    QSettings settings_;
};
