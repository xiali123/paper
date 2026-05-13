#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct TeamDashboardEntry {
    int id; QString member; QString category; QString role;
    qreal productivity; int tasks; bool active; QColor color;
};
class PaperTeamDashboard : public QWidget {
    Q_OBJECT
public:
    explicit PaperTeamDashboard(QWidget* parent = nullptr);
    void addEntry(const TeamDashboardEntry& entry);
    QList<TeamDashboardEntry> entries() const;
    int activeCount() const;
    qreal avgProductivity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void memberUpdated(int id, qreal productivity);
private slots:
    void onRefresh();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDashboardView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<TeamDashboardEntry> entries_;
    QSettings settings_;
    QPushButton* refreshBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
