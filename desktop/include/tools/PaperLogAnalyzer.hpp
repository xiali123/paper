#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct LogEntry {
    int id;
    QString source;
    QString level;
    QString message;
    int count;
    QString timestamp;
    QString category;
    qreal frequency;
    bool critical;
    QColor color;
};

class PaperLogAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogAnalyzer(QWidget* parent = nullptr);
    void addEntry(const LogEntry& entry);
    QList<LogEntry> entries() const;
    qreal avgFrequency() const;
    int criticalCount() const;
    QMap<QString, int> levelCounts() const;

signals:
    void logAnalyzed(int id, qreal frequency);

private slots:
    void onAnalyze();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawLogList(QPainter& p, const QRect& rect);
    void drawLevelChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<LogEntry> entries_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* levelCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
