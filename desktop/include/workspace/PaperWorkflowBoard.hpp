#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WorkflowEntry {
    int id; QString stage; QString category; QString status;
    int items; qreal throughput; QString owner; bool active; QColor color;
};
class PaperWorkflowBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperWorkflowBoard(QWidget* parent = nullptr);
    void addEntry(const WorkflowEntry& entry);
    QList<WorkflowEntry> entries() const;
    int activeCount() const;
    qreal avgThroughput() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void workflowUpdated(int id, qreal throughput);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawWorkflowView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WorkflowEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
