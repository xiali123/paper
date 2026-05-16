#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AssumptionEntry {
    int id; QString assumption; QString category; QString impact;
    qreal risk; int dependencies; bool validated; QColor color;
};
class PaperAssumptionMap : public QWidget {
    Q_OBJECT
public:
    explicit PaperAssumptionMap(QWidget* parent = nullptr);
    void addEntry(const AssumptionEntry& entry);
    QList<AssumptionEntry> entries() const;
    int validatedCount() const;
    qreal avgRisk() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void assumptionChecked(int id, qreal risk);
private slots:
    void onMap();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAssumptionGraph(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AssumptionEntry> entries_;
    QSettings settings_;
    QPushButton* mapBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
