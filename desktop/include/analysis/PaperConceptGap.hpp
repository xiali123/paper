#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct ConceptGapEntry {
    int id;
    QString concept;
    QString domain;
    QString category;
    qreal coverage;
    qreal similarity;
    bool covered;
    QColor color;
};

class PaperConceptGap : public QWidget {
    Q_OBJECT
public:
    explicit PaperConceptGap(QWidget* parent = nullptr);
    void addEntry(const ConceptGapEntry& entry);
    QList<ConceptGapEntry> entries() const;
    int coveredCount() const;
    qreal avgCoverage() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void gapAnalyzed(int id, qreal coverage);

private slots:
    void onAnalyze();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawGapList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<ConceptGapEntry> entries_;
    QSettings settings_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
