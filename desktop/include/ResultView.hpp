#pragma once

#include <QTableView>
#include <QStandardItemModel>
#include <QVector>

/**
 * @brief Paper search results table view
 */
class ResultView : public QTableView {
    Q_OBJECT

public:
    explicit ResultView(QWidget* parent = nullptr);
    void setPaperCount(int count);
    void clear();

signals:
    void paperSelected(int paperId);
    void exportRequested();

private slots:
    void onItemClicked(const QModelIndex& index);

private:
    void setupModel();

    QStandardItemModel* model_{nullptr};
    int paperCount_{0};
};
