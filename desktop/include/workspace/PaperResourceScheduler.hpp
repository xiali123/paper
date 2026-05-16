#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ResourceEntry {
    int id;
    QString resourceName;
    QString type;
    int capacity;
    int allocated;
    QString schedule;
    qreal utilization;
    bool available;
    QColor color;
};

class PaperResourceScheduler : public QWidget {
    Q_OBJECT
public:
    explicit PaperResourceScheduler(QWidget* parent = nullptr);
    void addEntry(const ResourceEntry& entry);
    QList<ResourceEntry> entries() const;
    qreal avgUtilization() const;
    int availableCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void resourceScheduled(int id, qreal utilization);

private slots:
    void onSchedule();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawResourceList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* scheduleBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ResourceEntry> entries_;
};
