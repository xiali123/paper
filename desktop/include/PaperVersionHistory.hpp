#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QPair>

struct VersionEntry {
    int version{-1};
    qint64 timestamp{0};
    QString author;
    QString summary;
    int paperId{-1};
};

class PaperVersionHistory : public QWidget {
    Q_OBJECT

public:
    explicit PaperVersionHistory(QWidget* parent = nullptr);

    void setPaperId(int paperId);
    void setVersions(const QList<VersionEntry>& versions);
    QList<VersionEntry> versions() const;

    void addVersion(const VersionEntry& entry);
    void clear();

    int currentVersion() const { return currentVersion_; }

signals:
    void versionSelected(int version);
    void compareRequested(int versionA, int versionB);
    void restoreRequested(int version);

private slots:
    void onVersionClicked(QListWidgetItem* item);
    void onCompare();
    void onRestore();
    void onRefresh();

private:
    void setupUI();
    void refreshList();

    QListWidget* versionList_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* compareBtn_{nullptr};
    QPushButton* restoreBtn_{nullptr};
    QPushButton* refreshBtn_{nullptr};

    QList<VersionEntry> versions_;
    int paperId_{-1};
    int currentVersion_{-1};
    int selectedVersionA_{-1};
    int selectedVersionB_{-1};
};
