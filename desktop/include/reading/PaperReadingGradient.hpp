#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct GradientEntry {
    int id; QString paper; QString category; QString level;
    qreal startProgress; qreal endProgress; qreal delta; bool advanced; QColor color;
};
class PaperReadingGradient : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingGradient(QWidget* parent = nullptr);
    void addEntry(const GradientEntry& entry);
    QList<GradientEntry> entries() const;
    int advancedCount() const;
    qreal avgDelta() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void gradientMeasured(int id, qreal delta);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGradientView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<GradientEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
