#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PatentEntry {
    int id; QString patent; QString category; QString status;
    qreal relevance; int citations; bool active; QColor color;
};
class PaperPatentTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperPatentTracker(QWidget* parent = nullptr);
    void addEntry(const PatentEntry& entry);
    QList<PatentEntry> entries() const;
    int activeCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void patentTracked(int id, qreal relevance);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPatentList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PatentEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
