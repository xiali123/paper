#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct CircuitEntry {
    int id; QString service; QString category; QString state;
    qreal failureRate; int requests; bool open; QColor color;
};
class PaperCircuitBreaker : public QWidget {
    Q_OBJECT
public:
    explicit PaperCircuitBreaker(QWidget* parent = nullptr);
    void addEntry(const CircuitEntry& entry);
    QList<CircuitEntry> entries() const;
    int openCount() const;
    qreal avgFailureRate() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void breakerTripped(int id, qreal failureRate);
private slots:
    void onMonitor();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawCircuitView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<CircuitEntry> entries_;
    QSettings settings_;
    QPushButton* monitorBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
