#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FallacyDetector2Entry {
    int id; QString argument; QString category; QString fallacy;
    qreal severity; int occurrences; bool critical; QColor color;
};
class PaperFallacyDetector2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperFallacyDetector2(QWidget* parent = nullptr);
    void addEntry(const FallacyDetector2Entry& entry);
    QList<FallacyDetector2Entry> entries() const;
    int criticalCount() const;
    qreal avgSeverity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void fallacyDetected(int id, qreal severity);
private slots:
    void onDetect();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFallacyView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FallacyDetector2Entry> entries_;
    QSettings settings_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
