#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>

struct SurveyEntry {
    int id;
    QString topic;
    QStringList papers;
    QString status;
    int questionCount;
    qreal coverage;
    QString notes;
    QColor color;
};

class PaperSurveyBuilder : public QWidget {
    Q_OBJECT
public:
    explicit PaperSurveyBuilder(QWidget* parent = nullptr);
    void addSurvey(const SurveyEntry& survey);
    QList<SurveyEntry> surveys() const;
    QMap<QString, int> statusCounts() const;
    qreal avgCoverage() const;
    int totalQuestions() const;

signals:
    void surveyAdded(int id, const QString& topic);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawSurveyList(QPainter& p, const QRect& rect);
    void drawStatusChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<SurveyEntry> surveys_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
