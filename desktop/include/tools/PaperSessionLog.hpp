#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SessionLogEntry {
    int id; QString action; QString category; QString user;
    qreal duration; int events; bool active; QColor color;
};
class PaperSessionLog : public QWidget {
    Q_OBJECT
public:
    explicit PaperSessionLog(QWidget* parent = nullptr);
    void addEntry(const SessionLogEntry& entry);
    QList<SessionLogEntry> entries() const;
    int activeCount() const;
    qreal totalDuration() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void sessionRecorded(int id, qreal duration);
private slots:
    void onRecord();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLogView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SessionLogEntry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
