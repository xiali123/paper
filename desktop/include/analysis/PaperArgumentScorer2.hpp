#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ArgumentScorer2Entry {
    int id; QString argument; QString category; QString type;
    qreal score; int premises; bool valid; QColor color;
};
class PaperArgumentScorer2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentScorer2(QWidget* parent = nullptr);
    void addEntry(const ArgumentScorer2Entry& entry);
    QList<ArgumentScorer2Entry> entries() const;
    int validCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void argumentScored(int id, qreal score);
private slots:
    void onScore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScorerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArgumentScorer2Entry> entries_;
    QSettings settings_;
    QPushButton* scoreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
