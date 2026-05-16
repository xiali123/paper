#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HypothesisEntry {
    int id; QString hypothesis; QString category; QString status;
    qreal confidence; int tests; bool proven; QColor color;
};
class PaperHypothesisGrid : public QWidget {
    Q_OBJECT
public:
    explicit PaperHypothesisGrid(QWidget* parent = nullptr);
    void addEntry(const HypothesisEntry& entry);
    QList<HypothesisEntry> entries() const;
    int provenCount() const;
    qreal avgConfidence() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void hypothesisTested(int id, qreal confidence);
private slots:
    void onTest();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGrid(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HypothesisEntry> entries_;
    QSettings settings_;
    QPushButton* testBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
