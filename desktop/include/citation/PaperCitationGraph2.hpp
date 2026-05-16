#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationGraph2Entry {
    int id; QString paper; QString category; QString cluster;
    qreal impact; int citations; bool influential; QColor color;
};
class PaperCitationGraph2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationGraph2(QWidget* parent = nullptr);
    void addEntry(const CitationGraph2Entry& entry);
    QList<CitationGraph2Entry> entries() const;
    int influentialCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void nodeSelected(int id, qreal impact);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGraphView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationGraph2Entry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
