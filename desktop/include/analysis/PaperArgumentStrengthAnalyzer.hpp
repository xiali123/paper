#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct ArgumentEntry {
    int id;
    QString claim;
    QString evidence;
    qreal strength;
    QString type;
    qreal confidence;
    int supportingPoints;
    QString counterArgument;
    QColor color;
};

class PaperArgumentStrengthAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperArgumentStrengthAnalyzer(QWidget* parent = nullptr);
    void addArgument(const ArgumentEntry& entry);
    QList<ArgumentEntry> arguments() const;
    QMap<QString, int> typeCounts() const;
    qreal avgStrength() const;
    int strongCount() const;

signals:
    void argumentAnalyzed(int id, qreal strength);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAnalyze();
    void onClear();
    void drawArgumentList(QPainter& p, const QRect& rect);
    void drawStrengthChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<ArgumentEntry> arguments_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
