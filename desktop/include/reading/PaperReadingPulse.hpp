#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PulseEntry {
    int id; QString paper; QString category; QString rhythm;
    qreal speed; qreal focus; qreal retention; bool steady; QColor color;
};
class PaperReadingPulse : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingPulse(QWidget* parent = nullptr);
    void addEntry(const PulseEntry& entry);
    QList<PulseEntry> entries() const;
    int steadyCount() const;
    qreal avgFocus() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pulseMeasured(int id, qreal focus);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPulseView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PulseEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
