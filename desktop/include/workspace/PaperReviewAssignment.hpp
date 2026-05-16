#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct ReviewAssignmentEntry {
    int id;
    QString paperTitle;
    QString reviewer;
    QString status;
    qreal expertise;
    int turnaround;
    QString field;
    qreal workload;
    QString priority;
    bool completed;
    QColor color;
};

class PaperReviewAssignment : public QWidget {
    Q_OBJECT
public:
    explicit PaperReviewAssignment(QWidget* parent = nullptr);
    void addEntry(const ReviewAssignmentEntry& entry);
    QList<ReviewAssignmentEntry> entries() const;
    int completedCount() const;
    qreal avgExpertise() const;
    QMap<QString, int> statusCounts() const;

signals:
    void assignmentCreated(int id, const QString& reviewer);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAssign();
    void onClear();
    void drawAssignmentList(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* priorityCombo_;
    QPushButton* assignBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<ReviewAssignmentEntry> entries_;
    QSettings settings_;
};
