#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QList>

struct ComparePaper {
    int id{-1};
    QString title;
    QString authors;
    int year{0};
    QString journal;
    QString doi;
    QString abstractText;
    QString keywords;
    int citationCount{0};
    double impactFactor{0.0};
    int pageCount{0};
    QString venue;
    QString publisher;
};

class PaperComparisonSlider : public QWidget {
    Q_OBJECT

public:
    explicit PaperComparisonSlider(QWidget* parent = nullptr);

    void setPapers(const QList<ComparePaper>& papers);
    void addPaper(const ComparePaper& paper);
    void clear();
    int paperCount() const;

signals:
    void paperSelected(int paperId);
    void fieldCompared(const QString& field);

private slots:
    void onAddPaper();
    void onRemovePaper();
    void onCompareField(int row, int col);

private:
    void setupUI();
    void refreshTable();
    void highlightDifferences();

    QTableWidget* compareTable_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QComboBox* fieldCombo_{nullptr};

    QList<ComparePaper> papers_;
    QStringList fieldLabels_;
};
