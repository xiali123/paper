#include "ResultView.hpp"
#include <QHeaderView>
#include <QStandardItemModel>
#include <QDebug>

ResultView::ResultView(QWidget* parent) : QTableView(parent), paperCount_(0) {
    setupModel();
    setupStyles();

    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setAlternatingRowColors(true);

    horizontalHeader()->setStretchLastSection(true);
    verticalHeader()->setVisible(false);

    setShowGrid(false);
    setStyleSheet(
        "QTableView {"
        "  background-color: rgba(255, 255, 255, 0.95);"
        "  border-radius: 15px;"
        "  border: none;"
        "  gridline-color: transparent;"
        "  selection-background-color: rgba(99, 102, 241, 0.2);"
        "  selection-color: #111827;"
        "}"
        "QTableView::item {"
        "  border: none;"
        "  padding: 12px 8px;"
        "  border-bottom: 1px solid #f3f4f6;"
        "}"
        "QTableView::item:hover {"
        "  background-color: rgba(99, 102, 241, 0.1);"
        "}"
        "QTableView::item:selected {"
        "  background-color: rgba(99, 102, 241, 0.2);"
        "}"
    );

    connect(this, &QTableView::clicked,
            this, &ResultView::onItemClicked);
}

void ResultView::setupModel() {
    model_ = new QStandardItemModel(this);
    model_->setHorizontalHeaderLabels({
        "ID", "Title", "Journal", "Year", "CCF Level"
    });

    setModel(model_);

    // Set column widths
    setColumnWidth(0, 60);
    setColumnWidth(1, 400);
    setColumnWidth(2, 200);
    setColumnWidth(3, 80);
    setColumnWidth(4, 100);

    // Header styling
    horizontalHeader()->setStyleSheet(
        "QHeaderView::section {"
        "  background-color: #f9fafb;"
        "  color: #374151;"
        "  padding: 16px 12px;"
        "  border: none;"
        "  border-bottom: 2px solid #e5e7eb;"
        "  font-weight: 700;"
        "  font-size: 10pt;"
        "}"
    );
}

void ResultView::setupStyles() {
    // Additional styling can be added here
}

QString ResultView::getLevelStyle(const QString& level) const {
    if (level.toUpper() == "A") {
        return "color: #991b1b; background-color: #fecaca; "
               "padding: 4px 12px; border-radius: 6px; font-weight: bold;";
    } else if (level.toUpper() == "B") {
        return "color: #9a3412; background-color: #fed7aa; "
               "padding: 4px 12px; border-radius: 6px; font-weight: bold;";
    } else if (level.toUpper() == "C") {
        return "color: #374151; background-color: #d1d5db; "
               "padding: 4px 12px; border-radius: 6px; font-weight: bold;";
    }
    return "color: #6b7280; background-color: #f3f4f6; "
           "padding: 4px 12px; border-radius: 6px;";
}

void ResultView::setPaperCount(int count) {
    paperCount_ = count;

    // Clear existing
    model_->removeRows(0, model_->rowCount());

    // Add demo data with modern styling
    for (int i = 0; i < std::min(count, 15); ++i) {
        addPaper(
            i + 1,
            QString("Paper %1: Deep Learning Advances in Computer Vision").arg(i + 1),
            "CVPR 2024",
            "2024",
            (i % 3 == 0) ? "A" : (i % 3 == 1) ? "B" : "C"
        );
    }
}

void ResultView::addPaper(int id, const QString& title, const QString& journal,
                          const QString& year, const QString& level) {
    QList<QStandardItem*> row;

    // ID
    QStandardItem* idItem = new QStandardItem(QString::number(id));
    idItem->setTextAlignment(Qt::AlignCenter);
    row << idItem;

    // Title
    QStandardItem* titleItem = new QStandardItem(title);
    titleItem->setForeground(QBrush(QColor(17, 24, 39)));
    row << titleItem;

    // Journal
    QStandardItem* journalItem = new QStandardItem(journal);
    journalItem->setForeground(QBrush(QColor(75, 85, 99)));
    row << journalItem;

    // Year
    QStandardItem* yearItem = new QStandardItem(year);
    yearItem->setTextAlignment(Qt::AlignCenter);
    yearItem->setForeground(QBrush(QColor(107, 114, 128)));
    row << yearItem;

    // CCF Level with badge styling
    QStandardItem* levelItem = new QStandardItem(level);
    levelItem->setTextAlignment(Qt::AlignCenter);

    // Apply level-specific colors
    QString levelStyle = getLevelStyle(level);
    levelItem->setData(levelStyle, Qt::UserRole + 1);

    if (level.toUpper() == "A") {
        levelItem->setForeground(QBrush(QColor(153, 27, 27)));
        levelItem->setBackground(QBrush(QColor(254, 202, 202)));
    } else if (level.toUpper() == "B") {
        levelItem->setForeground(QBrush(QColor(154, 52, 18)));
        levelItem->setBackground(QBrush(QColor(254, 215, 170)));
    } else if (level.toUpper() == "C") {
        levelItem->setForeground(QBrush(QColor(55, 65, 81)));
        levelItem->setBackground(QBrush(QColor(209, 213, 219)));
    }

    row << levelItem;

    model_->appendRow(row);
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
