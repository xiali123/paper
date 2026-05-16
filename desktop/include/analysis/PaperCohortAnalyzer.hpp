#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct CohortEntry {
    int id;
    QString subject;
    QString group;
    QString category;
    QString outcome;
    qreal score;
    int age;
    bool retained;
    QColor color;
};

class PaperCohortAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperCohortAnalyzer(QWidget* parent = nullptr);
    void addEntry(const CohortEntry& entry);
    QList<CohortEntry> entries() const;
    int retainedCount() const;
    qreal avgScore() const;
    QMap<QString, int> groupCounts() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void cohortAnalyzed(int id, qreal score);

private slots:
    void onAnalyze();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCohortList(QPainter& p, const QRect& rect);
    void drawGroupChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<CohortEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* groupCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
