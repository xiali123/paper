#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct ArgumentNode {
    int id{-1};
    QString claim;
    QString type; // "premise", "evidence", "conclusion", "counter"
    QString strength; // "strong", "moderate", "weak"
    QStringList supports;
    QStringList attacks;
    QColor color;
};

class PaperArgumentParser : public QWidget {
    Q_OBJECT

public:
    explicit PaperArgumentParser(QWidget* parent = nullptr);

    void addArgument(const ArgumentNode& arg);
    QList<ArgumentNode> arguments() const;
    QMap<QString, int> typeCounts() const;
    int strongArguments() const;

signals:
    void argumentParsed(int argId, const QString& type);
    void analysisComplete(int total);

private slots:
    void onAdd();
    void onAnalyze();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawArgumentGraph(QPainter& p, const QRect& rect);
    void drawStrengthChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* analyzeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ArgumentNode> arguments_;
    int selectedArg_{-1};
    QSettings settings_;
};
