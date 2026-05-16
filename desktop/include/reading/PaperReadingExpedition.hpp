#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ExpeditionEntry {
    int id; QString topic; QString category; QString terrain;
    qreal progress; int discoveries; bool complete; QColor color;
};
class PaperReadingExpedition : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingExpedition(QWidget* parent = nullptr);
    void addEntry(const ExpeditionEntry& entry);
    QList<ExpeditionEntry> entries() const;
    int completeCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void expeditionDone(int id, qreal progress);
private slots:
    void onExplore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawExpeditionMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ExpeditionEntry> entries_;
    QSettings settings_;
    QPushButton* exploreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
