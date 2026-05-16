#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LogEntry {
    int id; QString source; QString category; QString level;
    int count; qreal size; QString lastRotate; bool compressed; QColor color;
};
class PaperLogRotator : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogRotator(QWidget* parent = nullptr);
    void addEntry(const LogEntry& entry);
    QList<LogEntry> entries() const;
    int compressedCount() const;
    qreal avgSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void logRotated(int id, qreal size);
private slots:
    void onRotate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLogList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogEntry> entries_;
    QSettings settings_;
    QPushButton* rotateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
