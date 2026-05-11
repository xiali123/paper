#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct MilestoneEntry {
    int id;
    QString name;
    QString category;
    QString status;
    QString dependency;
    qreal progress;
    int daysLeft;
    bool critical;
    QColor color;
};

class PaperMilestoneTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperMilestoneTracker(QWidget* parent = nullptr);
    void addEntry(const MilestoneEntry& entry);
    QList<MilestoneEntry> entries() const;
    int criticalCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void milestoneCreated(int id, qreal progress);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawMilestoneList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<MilestoneEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
