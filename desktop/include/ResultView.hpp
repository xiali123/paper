#pragma once

#include <QTableView>
#include <QStandardItemModel>
#include <QVector>
#include <QStyledItemDelegate>

/**
 * @brief Modern paper search results table view
 *
 * Features:
 * - Card-like row styling
 * - Hover effects
 * - Modern CCF level badges
 * - Smooth selection animations
 */
class ResultView : public QTableView {
    Q_OBJECT

public:
    explicit ResultView(QWidget* parent = nullptr);
    void setPaperCount(int count);
    void clear();
    void addPaper(int id, const QString& title, const QString& journal,
                  const QString& year, const QString& level);

signals:
    void paperSelected(int paperId);
    void exportRequested();

private slots:
    void onItemClicked(const QModelIndex& index);

private:
    void setupModel();
    void setupStyles();
    QString getLevelStyle(const QString& level) const;

    QStandardItemModel* model_{nullptr};
    int paperCount_{0};
};
