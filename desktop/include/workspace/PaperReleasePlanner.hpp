#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReleaseEntry {
    int id; QString version; QString category; QString status;
    int features; qreal progress; QString date; bool released; QColor color;
};
class PaperReleasePlanner : public QWidget {
    Q_OBJECT
public:
    explicit PaperReleasePlanner(QWidget* parent = nullptr);
    void addEntry(const ReleaseEntry& entry);
    QList<ReleaseEntry> entries() const;
    int releasedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void releaseUpdated(int id, qreal progress);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawReleaseList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReleaseEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
