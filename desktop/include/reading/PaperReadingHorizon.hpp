#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct HorizonEntry {
    int id; QString field; QString category; QString trend;
    qreal distance; int papers; bool emerging; QColor color;
};
class PaperReadingHorizon : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingHorizon(QWidget* parent = nullptr);
    void addEntry(const HorizonEntry& entry);
    QList<HorizonEntry> entries() const;
    int emergingCount() const;
    qreal avgDistance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void horizonScanned(int id, qreal distance);
private slots:
    void onScan();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawHorizonView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<HorizonEntry> entries_;
    QSettings settings_;
    QPushButton* scanBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
