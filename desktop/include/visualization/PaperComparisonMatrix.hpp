#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QList>
#include "core/PaperTypes.hpp"

class PaperComparisonMatrix : public QWidget {
    Q_OBJECT

public:
    explicit PaperComparisonMatrix(QWidget* parent = nullptr);

    void setPapers(const QList<Paper>& papers);
    QList<Paper> papers() const;

    void setComparisonFields(const QStringList& fields);
    QStringList comparisonFields() const;

    void highlightDifferences(bool enable);
    void setMaxPapers(int max) { maxPapers_ = max; }

signals:
    void paperClicked(int paperId);
    void exportRequested(const QString& format);

private slots:
    void onCellClicked(int row, int col);
    void onHighlightToggled(bool enabled);
    void onExport();

private:
    void setupUI();
    void buildMatrix();
    void applyHighlighting();

    QTableWidget* matrixTable_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* highlightBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};

    QList<Paper> papers_;
    QStringList fields_;
    bool highlightEnabled_{true};
    int maxPapers_{10};
};
