#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct IncidentEntry {
    int id; QString title; QString category; QString severity;
    QString assignee; qreal mttr; QString status; bool resolved; QColor color;
};
class PaperIncidentBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperIncidentBoard(QWidget* parent = nullptr);
    void addEntry(const IncidentEntry& entry);
    QList<IncidentEntry> entries() const;
    int resolvedCount() const;
    qreal avgMttr() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void incidentLogged(int id, qreal mttr);
private slots:
    void onLog();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawIncidentList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<IncidentEntry> entries_;
    QSettings settings_;
    QPushButton* logBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
