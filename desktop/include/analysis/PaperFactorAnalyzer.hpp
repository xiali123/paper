#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct FactorEntry {
    int id;
    QString factor;
    QString category;
    qreal variance;
    qreal eigenvalue;
    int loadings;
    bool dominant;
    QColor color;
};

class PaperFactorAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperFactorAnalyzer(QWidget* parent = nullptr);
    void addEntry(const FactorEntry& entry);
    QList<FactorEntry> entries() const;
    int dominantCount() const;
    qreal totalVariance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void factorAnalyzed(int id, qreal variance);
private slots:
    void onAnalyze();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFactorList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FactorEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
