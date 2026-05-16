#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct AlertEntry {
    int id{-1};
    QString title;
    QString category; // "citation", "publication", "keyword", "author", "trend"
    QString severity; // "info", "warning", "critical"
    QDate date;
    QString description;
    bool read{false};
    QColor color;
};

class PaperAlertMonitor : public QWidget {
    Q_OBJECT

public:
    explicit PaperAlertMonitor(QWidget* parent = nullptr);

    void addAlert(const AlertEntry& alert);
    QList<AlertEntry> alerts() const;
    int unreadCount() const;
    QMap<QString, int> categoryCounts() const;
    QMap<QString, int> severityCounts() const;

signals:
    void alertTriggered(int alertId, const QString& severity);
    void unreadChanged(int count);

private slots:
    void onAdd();
    void onMarkRead();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawAlertList(QPainter& p, const QRect& rect);
    void drawSeverityChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* markReadBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<AlertEntry> alerts_;
    QSettings settings_;
};
