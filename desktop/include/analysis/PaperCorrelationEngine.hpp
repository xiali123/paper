#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct CorrelationEntry {
    int id;
    QString varX;
    QString varY;
    QString category;
    qreal coefficient;
    qreal pValue;
    int samples;
    bool significant;
    QColor color;
};

class PaperCorrelationEngine : public QWidget {
    Q_OBJECT
public:
    explicit PaperCorrelationEngine(QWidget* parent = nullptr);
    void addEntry(const CorrelationEntry& entry);
    QList<CorrelationEntry> entries() const;
    int significantCount() const;
    qreal avgCoefficient() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void correlationFound(int id, qreal coefficient);

private slots:
    void onAnalyze();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCorrelationList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<CorrelationEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
