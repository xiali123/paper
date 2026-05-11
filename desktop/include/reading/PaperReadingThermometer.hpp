#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ThermEntry {
    int id; QString paper; QString category; QString level;
    qreal progress; qreal temperature; int pages; bool hot; QColor color;
};
class PaperReadingThermometer : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingThermometer(QWidget* parent = nullptr);
    void addEntry(const ThermEntry& entry);
    QList<ThermEntry> entries() const;
    int hotCount() const;
    qreal avgTemperature() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void readingMeasured(int id, qreal temperature);
private slots:
    void onMeasure();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawThermList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ThermEntry> entries_;
    QSettings settings_;
    QPushButton* measureBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
