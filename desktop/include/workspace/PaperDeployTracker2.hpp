#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DeployTracker2Entry {
    int id; QString service; QString category; QString environment;
    qreal uptime; int deployments; bool healthy; QColor color;
};
class PaperDeployTracker2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperDeployTracker2(QWidget* parent = nullptr);
    void addEntry(const DeployTracker2Entry& entry);
    QList<DeployTracker2Entry> entries() const;
    int healthyCount() const;
    qreal avgUptime() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void deployCompleted(int id, qreal uptime);
private slots:
    void onDeploy();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawTrackerView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DeployTracker2Entry> entries_;
    QSettings settings_;
    QPushButton* deployBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
