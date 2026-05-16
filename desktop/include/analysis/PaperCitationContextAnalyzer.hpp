#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct CitationContextEntry {
    int id;
    QString sourcePaper;
    QString targetPaper;
    QString context;
    QString contextType;
    qreal relevance;
    int position;
    QString surrounding;
    QColor color;
};

class PaperCitationContextAnalyzer : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationContextAnalyzer(QWidget* parent = nullptr);
    void addContext(const CitationContextEntry& entry);
    QList<CitationContextEntry> contexts() const;
    QMap<QString, int> typeCounts() const;
    qreal avgRelevance() const;
    int uniquePairs() const;

signals:
    void contextAnalyzed(int id, const QString& contextType);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAnalyze();
    void onClear();
    void drawContextList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<CitationContextEntry> contexts_;
    QPushButton* analyzeBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
