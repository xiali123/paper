#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct KeywordEvolutionEntry {
    int id;
    QString keyword;
    int year;
    qreal frequency;
    QString trend;
    int rank;
    QString category;
    qreal growth;
    QString relatedTerm;
    QColor color;
};

class PaperKeywordEvolution : public QWidget {
    Q_OBJECT
public:
    explicit PaperKeywordEvolution(QWidget* parent = nullptr);
    void addEntry(const KeywordEvolutionEntry& entry);
    QList<KeywordEvolutionEntry> entries() const;
    qreal avgGrowth() const;
    int risingCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void evolutionGenerated(int id, qreal growth);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawTrendList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* categoryCombo_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<KeywordEvolutionEntry> entries_;
    QSettings settings_;
};
