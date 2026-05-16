#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct CitationAnalysis {
    int id;
    QString paperTitle;
    QString citationType;
    int selfCitations;
    int crossCitations;
    qreal hIndex;
    QString field;
    int yearSpan;
    bool influential;
    QColor color;
};

class PaperCitationAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationAnalyzer(QWidget* parent = nullptr);
    void addEntry(const CitationAnalysis& entry);
    QList<CitationAnalysis> entries() const;
    qreal avgHIndex() const;
    int influentialCount() const;
    QMap<QString, int> fieldCounts() const;

signals:
    void citationAnalyzed(int id, qreal hIndex);

private slots:
    void onAnalyze();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCitationList(QPainter& p, const QRect& rect);
    void drawFieldChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QComboBox* fieldCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<CitationAnalysis> entries_;
};
