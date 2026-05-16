#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct MethodologyEntry {
    int id{-1};
    QString name;
    QString type; // "quantitative", "qualitative", "mixed", "experimental"
    QString field;
    QString tools;
    int papersUsed{0};
    qreal rigorScore{0};
    QColor color;
};

class PaperMethodologyExtractor : public QWidget {
    Q_OBJECT

public:
    explicit PaperMethodologyExtractor(QWidget* parent = nullptr);

    void addMethodology(const MethodologyEntry& entry);
    QList<MethodologyEntry> methodologies() const;
    QMap<QString, int> typeCounts() const;
    qreal averageRigor() const;

signals:
    void methodologyExtracted(int id, const QString& type);
    void analysisComplete(int count);

private slots:
    void onAdd();
    void onAnalyze();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawMethodologyList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* analyzeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<MethodologyEntry> methodologies_;
    int selectedEntry_{-1};
    QSettings settings_;
};
