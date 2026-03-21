#include "ResultView.hpp"
#include <QHeaderView>
#include <QStandardItemModel>

ResultView::ResultView(QWidget* parent) : QTableView(parent), paperCount_(0) {
    setupModel();
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(true);

    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->setVisible(false);

    connect(this, &QTableView::clicked,
            this, &ResultView::onItemClicked);
}

void ResultView::setupModel() {
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({
        "ID", "Title", "Journal", "Year", "Level"
    });

    setModel(model_);

    // Set column widths
    setColumnWidth(0, 50);
    setColumnWidth(2, 200);
    setColumnWidth(3, 80);
    setColumnWidth(4, 60);
}

void ResultView::setPaperCount(int count) {
    paperCount_ = count;

    // Clear existing
    model_->removeRows(0, model_->rowCount());

    // Add demo data
    for (int i = 0; i < std::min(count, 10); ++i) {
        QList<QStandardItem*> row;
        row << new QStandardItem(QString::number(i + 1));
        row << new QStandardItem(QString("Paper %1: Deep Learning for Computer Vision").arg(i + 1));
        row << new QStandardItem("CVPR 2024");
        row << new QStandardItem("2024");
        row << new QStandardItem("A");

        model_->appendRow(row);
    }
}

void ResultView::clear() {
    model_->removeRows(0, model_->rowCount());
    paperCount_ = 0;
}

void ResultView::onItemClicked(const QModelIndex& index) {
    if (index.isValid() && index.row() < paperCount_) {
        emit paperSelected(index.row() + 1);
    }
}
