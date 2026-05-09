#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct DatasetEntry {
    int id{-1};
    QString name;
    QString format; // "csv", "json", "xml", "sql"
    QString source;
    QString status; // "local", "remote", "synced", "outdated"
    int records{0};
    qreal sizeMB{0};
    QDate lastUpdated;
    QString description;
    QColor color;
};

class PaperDatasetManager : public QWidget {
    Q_OBJECT

public:
    explicit PaperDatasetManager(QWidget* parent = nullptr);

    void addDataset(const DatasetEntry& dataset);
    QList<DatasetEntry> datasets() const;
    QMap<QString, int> formatCounts() const;
    QMap<QString, int> statusCounts() const;
    qreal totalSize() const;

signals:
    void datasetAdded(int id);
    void syncCompleted(int datasets);

private slots:
    void onAdd();
    void onSync();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawDatasetList(QPainter& p, const QRect& rect);
    void drawFormatChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* syncBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<DatasetEntry> datasets_;
    QSettings settings_;
};
