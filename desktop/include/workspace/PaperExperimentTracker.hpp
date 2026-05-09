#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct ExperimentEntry {
    int id{-1};
    QString name;
    QString hypothesis;
    QString status; // "planned", "running", "completed", "failed"
    QDate startDate;
    QDate endDate;
    qreal progress{0};
    QStringList parameters;
    QString result;
    QColor color;
};

class PaperExperimentTracker : public QWidget {
    Q_OBJECT

public:
    explicit PaperExperimentTracker(QWidget* parent = nullptr);

    void addExperiment(const ExperimentEntry& experiment);
    QList<ExperimentEntry> experiments() const;
    QMap<QString, int> statusCounts() const;
    int activeExperiments() const;
    qreal completionRate() const;

signals:
    void experimentStarted(int id);
    void experimentCompleted(int id, const QString& result);

private slots:
    void onAdd();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawExperimentCards(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ExperimentEntry> experiments_;
    QSettings settings_;
};
