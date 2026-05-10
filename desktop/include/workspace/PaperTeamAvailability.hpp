#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct TeamAvailEntry {
    int id;
    QString member;
    QString role;
    qreal availability;
    int tasksActive;
    int tasksCompleted;
    QString specialization;
    qreal workload;
    QString status;
    int capacity;
    QColor color;
};

class PaperTeamAvailability : public QWidget {
    Q_OBJECT
public:
    explicit PaperTeamAvailability(QWidget* parent = nullptr);
    void addEntry(const TeamAvailEntry& entry);
    QList<TeamAvailEntry> entries() const;
    qreal avgAvailability() const;
    int availableCount() const;
    QMap<QString, int> roleCounts() const;

signals:
    void availabilityUpdated(int id, qreal avail);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onRefresh();
    void onClear();
    void drawMemberList(QPainter& p, const QRect& rect);
    void drawRoleChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* roleCombo_;
    QPushButton* refreshBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<TeamAvailEntry> entries_;
    QSettings settings_;
};
