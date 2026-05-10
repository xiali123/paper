#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct ScorecardEntry {
    int id;
    QString userName;
    qreal comprehension;
    qreal consistency;
    qreal depth;
    qreal breadth;
    qreal overall;
    QString grade;
    int papersRead;
    QString period;
    bool excellent;
    QColor color;
};

class PaperReadingScorecard : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingScorecard(QWidget* parent = nullptr);
    void addEntry(const ScorecardEntry& entry);
    QList<ScorecardEntry> entries() const;
    qreal avgOverall() const;
    int excellentCount() const;
    QMap<QString, int> gradeCounts() const;

signals:
    void scorecardUpdated(int id, qreal overall);

private slots:
    void onUpdate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawScorecardList(QPainter& p, const QRect& rect);
    void drawGradeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<ScorecardEntry> entries_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
