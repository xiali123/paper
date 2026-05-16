#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VendorScorecardEntry {
    int id; QString vendor; QString category; QString metric;
    qreal score; int reviews; bool recommended; QColor color;
};
class PaperVendorScorecard : public QWidget {
    Q_OBJECT
public:
    explicit PaperVendorScorecard(QWidget* parent = nullptr);
    void addEntry(const VendorScorecardEntry& entry);
    QList<VendorScorecardEntry> entries() const;
    int recommendedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void vendorScored(int id, qreal score);
private slots:
    void onScore();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawScorecardView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VendorScorecardEntry> entries_;
    QSettings settings_;
    QPushButton* scoreBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
