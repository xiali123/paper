#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct SimilarityEntry {
    int id;
    QString paperA;
    QString paperB;
    qreal similarity;
    QString dimension;
    qreal confidence;
    QString method;
    int sharedTerms;
    QColor color;
};

class PaperSemanticSimilarityMatrix : public QWidget {
    Q_OBJECT
public:
    explicit PaperSemanticSimilarityMatrix(QWidget* parent = nullptr);
    void addEntry(const SimilarityEntry& entry);
    QList<SimilarityEntry> entries() const;
    qreal avgSimilarity() const;
    int highPairs() const;
    QMap<QString, int> methodCounts() const;

signals:
    void similarityComputed(int id, qreal similarity);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onCompute();
    void onClear();
    void drawMatrixGrid(QPainter& p, const QRect& rect);
    void drawMethodChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<SimilarityEntry> entries_;
    QPushButton* computeBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
