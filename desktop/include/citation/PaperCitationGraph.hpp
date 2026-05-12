#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CitationGraphEntry {
    int id; QString paper; QString category; QString cites;
    qreal impact; int depth; bool hub; QColor color;
};
class PaperCitationGraph : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationGraph(QWidget* parent = nullptr);
    void addEntry(const CitationGraphEntry& entry);
    QList<CitationGraphEntry> entries() const;
    int hubCount() const;
    qreal avgImpact() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void nodeSelected(int id, qreal impact);
private slots:
    void onBuild();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGraph(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CitationGraphEntry> entries_;
    QSettings settings_;
    QPushButton* buildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
