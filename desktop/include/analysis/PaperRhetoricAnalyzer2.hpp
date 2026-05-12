#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct RhetoricAnalyzer2Entry {
    int id; QString passage; QString category; QString device;
    qreal impact; int frequency; bool persuasive; QColor color;
};
class PaperRhetoricAnalyzer2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperRhetoricAnalyzer2(QWidget* parent = nullptr);
    void addEntry(const RhetoricAnalyzer2Entry& entry);
    QList<RhetoricAnalyzer2Entry> entries() const;
    int persuasiveCount() const;
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
    void drawAnalyzerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<RhetoricAnalyzer2Entry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
