#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct DeploymentEntry {
    int id;
    QString appName;
    QString environment;
    QString status;
    QString version;
    qreal uptime;
    int requests;
    qreal latency;
    bool healthy;
    QColor color;
};

class PaperDeploymentTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperDeploymentTracker(QWidget* parent = nullptr);
    void addEntry(const DeploymentEntry& entry);
    QList<DeploymentEntry> entries() const;
    int healthyCount() const;
    qreal avgUptime() const;
    QMap<QString, int> envCounts() const;

signals:
    void deploymentTracked(int id, qreal uptime);

private slots:
    void onDeploy();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawDeploymentList(QPainter& p, const QRect& rect);
    void drawEnvChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* deployBtn_;
    QPushButton* clearBtn_;
    QComboBox* envCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<DeploymentEntry> entries_;
};
