#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct ResourceEntry {
    int id{-1};
    QString name;
    QString type; // "paper", "dataset", "tool", "compute"
    QString status; // "available", "in-use", "reserved"
    QString assignee;
    QDate deadline;
    int priority{0}; // 1-5
    QString notes;
    QColor color;
};

class PaperResourceAllocator : public QWidget {
    Q_OBJECT

public:
    explicit PaperResourceAllocator(QWidget* parent = nullptr);

    void addResource(const ResourceEntry& resource);
    QList<ResourceEntry> resources() const;
    QMap<QString, int> typeCounts() const;
    QMap<QString, int> statusCounts() const;
    int availableCount() const;

signals:
    void resourceAllocated(int id, const QString& assignee);
    void resourceFreed(int id);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawResourceCards(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ResourceEntry> resources_;
    QSettings settings_;
};
