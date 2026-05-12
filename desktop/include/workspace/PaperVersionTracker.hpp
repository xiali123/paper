#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VersionEntry {
    int id; QString document; QString category; QString version;
    int changes; qreal diff; QString date; bool latest; QColor color;
};
class PaperVersionTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperVersionTracker(QWidget* parent = nullptr);
    void addEntry(const VersionEntry& entry);
    QList<VersionEntry> entries() const;
    int latestCount() const;
    qreal avgDiff() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void versionTracked(int id, qreal diff);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVersionList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VersionEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
