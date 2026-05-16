#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HypothesisGrid2Entry {
    int id; QString hypothesis; QString category; QString status;
    qreal confidence; int tests; bool confirmed; QColor color;
};
class PaperHypothesisGrid2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperHypothesisGrid2(QWidget* parent = nullptr);
    void addEntry(const HypothesisGrid2Entry& entry);
    QList<HypothesisGrid2Entry> entries() const;
    int confirmedCount() const;
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
    void drawGridView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HypothesisGrid2Entry> entries_;
    QSettings settings_;
    QPushButton* testBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
