#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FlowStateEntry {
    int id; QString session; QString category; QString phase;
    qreal flowScore; int interruptions; bool optimal; QColor color;
};
class PaperReadingFlowState : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingFlowState(QWidget* parent = nullptr);
    void addEntry(const FlowStateEntry& entry);
    QList<FlowStateEntry> entries() const;
    int optimalCount() const;
    qreal avgFlowScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void flowAchieved(int id, qreal score);
private slots:
    void onTrack();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFlowChart(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FlowStateEntry> entries_;
    QSettings settings_;
    QPushButton* trackBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
