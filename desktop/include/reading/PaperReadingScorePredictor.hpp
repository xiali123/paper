#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ScorePrediction {
    int id;
    QString paperTitle;
    QString category;
    qreal predictedScore;
    qreal confidence;
    QString factors;
    int citations;
    qreal impactFactor;
    bool highConfidence;
    QColor color;
};

class PaperReadingScorePredictor : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingScorePredictor(QWidget* parent = nullptr);
    void addEntry(const ScorePrediction& entry);
    QList<ScorePrediction> entries() const;
    qreal avgPredicted() const;
    int highConfidenceCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void scorePredicted(int id, qreal predictedScore);

private slots:
    void onPredict();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawPredictionList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* predictBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ScorePrediction> entries_;
};
