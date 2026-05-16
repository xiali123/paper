#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct TaskAutoEntry {
    int id;
    QString taskName;
    QString trigger;
    QString action;
    QString category;
    QString schedule;
    bool enabled;
    bool recurring;
    QColor color;
};

class PaperTaskAutomator : public QWidget {
    Q_OBJECT
public:
    explicit PaperTaskAutomator(QWidget* parent = nullptr);
    void addEntry(const TaskAutoEntry& entry);
    QList<TaskAutoEntry> entries() const;
    int enabledCount() const;
    int recurringCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void taskCreated(int id, const QString& taskName);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawTaskList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<TaskAutoEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
