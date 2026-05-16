#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DeployEntry {
    int id; QString version; QString category; QString environment;
    qreal uptime; int deploys; bool stable; QColor color;
};
class PaperDeployTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperDeployTracker(QWidget* parent = nullptr);
    void addEntry(const DeployEntry& entry);
    QList<DeployEntry> entries() const;
    int stableCount() const;
    qreal avgUptime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void deployComplete(int id, qreal uptime);
private slots:
    void onDeploy();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDeployTimeline(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DeployEntry> entries_;
    QSettings settings_;
    QPushButton* deployBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
