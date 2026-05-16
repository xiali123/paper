#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct VersionEntry {
    int id;
    QString version;
    QString branch;
    QString category;
    QString tag;
    QString date;
    int changes;
    bool stable;
    bool latest;
    QColor color;
};

class PaperVersionTree : public QWidget {
    Q_OBJECT
public:
    explicit PaperVersionTree(QWidget* parent = nullptr);
    void addEntry(const VersionEntry& entry);
    QList<VersionEntry> entries() const;
    int stableCount() const;
    QMap<QString, int> branchCounts() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void versionAdded(int id, const QString& version);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawVersionList(QPainter& p, const QRect& rect);
    void drawBranchChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<VersionEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
