#pragma once
#include <QWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct MilestoneEntry {
    int id;
    QString name;
    QString milestoneType;
    int papersRequired;
    int papersCompleted;
    qreal progress;
    QString period;
    QString reward;
    int daysRemaining;
    bool achieved;
    QColor color;
};

class PaperReadingMilestone : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingMilestone(QWidget* parent = nullptr);
    void addEntry(const MilestoneEntry& entry);
    QList<MilestoneEntry> entries() const;
    int achievedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> typeCounts() const;
signals:
    void milestoneReached(int id, qreal progress);
private slots:
    void onAdd();
    void onClear();
private:
    void setupUI();
    void paintEvent(QPaintEvent*) override;
    void drawMilestoneList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<MilestoneEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* typeCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
