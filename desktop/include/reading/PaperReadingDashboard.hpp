#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct DashboardEntry {
    int id;
    QString userName;
    int papersRead;
    int papersTotal;
    qreal completionRate;
    QString period;
    int streakDays;
    qreal avgSpeed;
    QString topCategory;
    bool onTrack;
    QColor color;
};

class PaperReadingDashboard : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingDashboard(QWidget* parent = nullptr);
    void addEntry(const DashboardEntry& entry);
    QList<DashboardEntry> entries() const;
    qreal avgCompletion() const;
    int onTrackCount() const;
    QMap<QString, int> periodCounts() const;

signals:
    void dashboardUpdated(int id, qreal completionRate);

private slots:
    void onUpdate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawDashboardList(QPainter& p, const QRect& rect);
    void drawPeriodChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<DashboardEntry> entries_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
