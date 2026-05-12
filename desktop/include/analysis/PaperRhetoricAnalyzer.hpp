#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RhetoricEntry {
    int id; QString passage; QString category; QString device;
    qreal impact; int uses; bool effective; QColor color;
};
class PaperRhetoricAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperRhetoricAnalyzer(QWidget* parent = nullptr);
    void addEntry(const RhetoricEntry& entry);
    QList<RhetoricEntry> entries() const;
    int effectiveCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void deviceFound(int id, qreal impact);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRhetoricMap(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RhetoricEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
