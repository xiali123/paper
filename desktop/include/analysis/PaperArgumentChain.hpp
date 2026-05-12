#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ChainEntry {
    int id; QString premise; QString category; QString link;
    qreal strength; int steps; bool valid; QColor color;
};
class PaperArgumentChain : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentChain(QWidget* parent = nullptr);
    void addEntry(const ChainEntry& entry);
    QList<ChainEntry> entries() const;
    int validCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void chainComplete(int id, qreal strength);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawChainDiagram(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ChainEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
