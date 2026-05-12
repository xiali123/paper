#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ExperimentEntry {
    int id; QString name; QString category; QString phase;
    qreal result; int runs; bool success; QColor color;
};
class PaperExperimentLog : public QWidget {
    Q_OBJECT
public:
    explicit PaperExperimentLog(QWidget* parent = nullptr);
    void addEntry(const ExperimentEntry& entry);
    QList<ExperimentEntry> entries() const;
    int successCount() const;
    qreal avgResult() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void experimentLogged(int id, qreal result);
private slots:
    void onLog();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLogView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ExperimentEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
