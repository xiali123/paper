#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct AssumptionMap2Entry {
    int id; QString assumption; QString category; QString status;
    qreal risk; int dependencies; bool critical; QColor color;
};
class PaperAssumptionMap2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperAssumptionMap2(QWidget* parent = nullptr);
    void addEntry(const AssumptionMap2Entry& entry);
    QList<AssumptionMap2Entry> entries() const;
    int criticalCount() const;
    qreal avgRisk() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void assumptionValidated(int id, qreal risk);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMapView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<AssumptionMap2Entry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
