#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ContradictionMapEntry {
    int id; QString claim1; QString category; QString claim2;
    qreal conflict; int votes; bool resolved; QColor color;
};
class PaperContradictionMap : public QWidget {
    Q_OBJECT
public:
    explicit PaperContradictionMap(QWidget* parent = nullptr);
    void addEntry(const ContradictionMapEntry& entry);
    QList<ContradictionMapEntry> entries() const;
    int resolvedCount() const;
    qreal avgConflict() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void contradictionFound(int id, qreal conflict);
private slots:
    void onDetect();
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
    QList<ContradictionMapEntry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
