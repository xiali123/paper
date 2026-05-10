#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct CompetencyEntry {
    int id;
    QString topic;
    QString level;
    qreal score;
    int papersRead;
    QString category;
    qreal targetScore;
    bool achieved;
    QColor color;
};

class PaperReadingCompetency : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingCompetency(QWidget* parent = nullptr);
    void addEntry(const CompetencyEntry& entry);
    QList<CompetencyEntry> entries() const;
    int achievedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void competencyUpdated(int id, qreal score);

private slots:
    void onAssess();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCompetencyGrid(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* assessBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<CompetencyEntry> entries_;
};
