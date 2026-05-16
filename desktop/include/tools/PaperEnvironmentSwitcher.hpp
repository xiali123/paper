#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct EnvironmentEntry {
    int id;
    QString envName;
    QString baseUrl;
    QString region;
    int latency;
    bool healthy;
    int papersCached;
    QString apiVersion;
    QString lastChecked;
    bool active;
    QColor color;
};

class PaperEnvironmentSwitcher : public QWidget {
    Q_OBJECT
public:
    explicit PaperEnvironmentSwitcher(QWidget* parent = nullptr);
    void addEntry(const EnvironmentEntry& entry);
    QList<EnvironmentEntry> entries() const;
    qreal avgLatency() const;
    int healthyCount() const;
    QMap<QString, int> regionCounts() const;

signals:
    void environmentSwitched(int id, int latency);

private slots:
    void onSwitch();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawEnvList(QPainter& p, const QRect& rect);
    void drawRegionChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<EnvironmentEntry> entries_;
    QPushButton* switchBtn_;
    QPushButton* clearBtn_;
    QComboBox* regionCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
