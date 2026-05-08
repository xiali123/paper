#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QPair>

struct SimilarityPair {
    int paperA{-1};
    int paperB{-1};
    QString titleA;
    QString titleB;
    double score{0.0};
    QString method;
    QString details;
};

class PaperSimilarityWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperSimilarityWidget(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers); // id, text
    void setThreshold(double t) { threshold_ = t; }
    void setMethod(const QString& method);

    QList<SimilarityPair> computeSimilarities();
    QList<SimilarityPair> results() const { return results_; }

signals:
    void computationDone(int pairCount);
    void pairClicked(int paperA, int paperB);

private slots:
    void onCompute();
    void onMethodChanged(int index);
    void onCellClicked(int row, int col);
    void onExport();

private:
    void setupUI();
    double jaccardSim(const QString& a, const QString& b) const;
    double cosineSim(const QString& a, const QString& b) const;

    QTableWidget* table_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* computeBtn_{nullptr};
    QComboBox* methodCombo_{nullptr};

    QList<QPair<int, QString>> papers_;
    QList<SimilarityPair> results_;
    double threshold_{0.3};
    QString method_{"jaccard"};
};
