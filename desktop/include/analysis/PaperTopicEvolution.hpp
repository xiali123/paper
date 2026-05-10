#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct TopicEvolution {
    int id;
    QString topic;
    QString period;
    qreal frequency;
    qreal growth;
    int papers;
    QString trend;
    qreal impact;
    bool rising;
    QColor color;
};

class PaperTopicEvolution : public QWidget {
    Q_OBJECT
public:
    explicit PaperTopicEvolution(QWidget* parent = nullptr);
    void addEntry(const TopicEvolution& entry);
    QList<TopicEvolution> entries() const;
    int risingCount() const;
    qreal avgGrowth() const;
    QMap<QString, int> trendCounts() const;

signals:
    void topicEvolved(int id, qreal growth);

private slots:
    void onAnalyze();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawEvolutionChart(QPainter& p, const QRect& rect);
    void drawTrendChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* periodCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<TopicEvolution> entries_;
};
