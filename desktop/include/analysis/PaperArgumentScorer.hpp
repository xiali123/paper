#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ArgumentScoreEntry {
    int id; QString argument; QString category; QString stance;
    qreal strength; int evidence; bool sound; QColor color;
};
class PaperArgumentScorer : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentScorer(QWidget* parent = nullptr);
    void addEntry(const ArgumentScoreEntry& entry);
    QList<ArgumentScoreEntry> entries() const;
    int soundCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void argumentScored(int id, qreal strength);
private slots:
    void onScore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScoreView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ArgumentScoreEntry> entries_;
    QSettings settings_;
    QPushButton* scoreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
