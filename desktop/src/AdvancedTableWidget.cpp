#include "AdvancedTableWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QContextMenuEvent>
#include <QTimer>

AdvancedTableWidget::AdvancedTableWidget(QWidget* parent)
    : QTableWidget(parent)
{
    setAlternatingRowColors(true);
    setSelectionBehavior(QAbstractItemView::SelectRows);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSortingEnabled(true);
    horizontalHeader()->setStretchLastSection(true);
    horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        int id = rowIdAt(row);
        if (id >= 0) emit rowDoubleClicked(id);
    });

    connect(this, &QTableWidget::itemSelectionChanged, this, [this]() {
        QList<int> ids;
        for (auto* row : selectionModel()->selectedRows()) {
            int id = rowIdAt(row->row());
            if (id >= 0) ids << id;
        }
        emit selectionChanged(ids);
    });

    connect(horizontalHeader(), &QHeaderView::sectionClicked,
            this, &AdvancedTableWidget::onHeaderClicked);
}

void AdvancedTableWidget::configureColumns(const QList<ColumnConfig>& columns) {
    columns_ = columns;
    setColumnCount(columns.size());
    setHorizontalHeaderLabels([this]() {
        QStringList labels;
        for (const auto& c : columns_) labels << c.label;
        return labels;
    }());

    for (int i = 0; i < columns.size(); ++i) {
        if (columns[i].width > 0) setColumnWidth(i, columns[i].width);
        if (!columns[i].visible) hideColumn(i);
    }
}

void AdvancedTableWidget::addRow(const QMap<QString, QString>& data, int rowId) {
    int row = rowCount();
    insertRow(row);

    if (rowId < 0) rowId = nextRowId_++;
    rowIdMap_[rowId] = row;

    for (int i = 0; i < columns_.size(); ++i) {
        auto* item = new QTableWidgetItem(data.value(columns_[i].key));
        item->setData(Qt::UserRole, rowId);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        setItem(row, i, item);
    }

    applyFilters();
}

void AdvancedTableWidget::updateRow(int row, const QMap<QString, QString>& data) {
    for (int i = 0; i < columns_.size(); ++i) {
        auto* item = this->item(row, i);
        if (item) item->setText(data.value(columns_[i].key));
    }
}

void AdvancedTableWidget::removeRow(int row) {
    int id = rowIdAt(row);
    if (id >= 0) rowIdMap_.remove(id);
    QTableWidget::removeRow(row);
    // Rebuild rowIdMap
    rowIdMap_.clear();
    for (int r = 0; r < rowCount(); ++r) {
        auto* item = this->item(r, 0);
        if (item) rowIdMap_[item->data(Qt::UserRole).toInt()] = r;
    }
}

void AdvancedTableWidget::removeRowById(int rowId) {
    int row = findRowById(rowId);
    if (row >= 0) removeRow(row);
}

int AdvancedTableWidget::findRowById(int rowId) const {
    return rowIdMap_.value(rowId, -1);
}

int AdvancedTableWidget::rowIdAt(int row) const {
    auto* item = this->item(row, 0);
    return item ? item->data(Qt::UserRole).toInt() : -1;
}

void AdvancedTableWidget::setFilter(const QString& column, const QString& value) {
    if (value.isEmpty()) filters_.remove(column);
    else filters_[column] = value;
    applyFilters();
    emit filterChanged(filters_);
}

void AdvancedTableWidget::clearFilters() {
    filters_.clear();
    searchQuery_.clear();
    applyFilters();
}

void AdvancedTableWidget::setSearchQuery(const QString& query) {
    searchQuery_ = query.toLower();
    applyFilters();
}

void AdvancedTableWidget::sortyByColumn(const QString& key, Qt::SortOrder order) {
    for (int i = 0; i < columns_.size(); ++i) {
        if (columns_[i].key == key) {
            sortColumn_ = i;
            sortOrder_ = order;
            sortItems(i, order);
            return;
        }
    }
}

QList<int> AdvancedTableWidget::visibleRowIds() const {
    QList<int> ids;
    for (int r = 0; r < rowCount(); ++r) {
        if (!isRowHidden(r)) {
            int id = rowIdAt(r);
            if (id >= 0) ids << id;
        }
    }
    return ids;
}

int AdvancedTableWidget::visibleRowCount() const {
    int count = 0;
    for (int r = 0; r < rowCount(); ++r) {
        if (!isRowHidden(r)) count++;
    }
    return count;
}

void AdvancedTableWidget::exportToClipboard() const {
    QStringList lines;

    // Header
    QStringList headers;
    for (const auto& c : columns_) {
        if (c.visible) headers << c.label;
    }
    lines << headers.join("\t");

    // Rows
    for (int r = 0; r < rowCount(); ++r) {
        if (isRowHidden(r)) continue;
        QStringList row;
        for (int c = 0; c < columns_.size(); ++c) {
            if (columns_[c].visible) {
                auto* item = this->item(r, c);
                row << (item ? item->text() : "");
            }
        }
        lines << row.join("\t");
    }

    QApplication::clipboard()->setText(lines.join("\n"));
}

void AdvancedTableWidget::onHeaderClicked(int logicalIndex) {
    if (sortColumn_ == logicalIndex) {
        sortOrder_ = (sortOrder_ == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
    } else {
        sortColumn_ = logicalIndex;
        sortOrder_ = Qt::AscendingOrder;
    }
    sortItems(logicalIndex, sortOrder_);
}

void AdvancedTableWidget::onFilterChanged() {
    applyFilters();
}

void AdvancedTableWidget::applyFilters() {
    for (int r = 0; r < rowCount(); ++r) {
        bool visible = true;

        // Search query
        if (!searchQuery_.isEmpty()) {
            bool found = false;
            for (int c = 0; c < columns_.size() && !found; ++c) {
                auto* item = this->item(r, c);
                if (item && item->text().toLower().contains(searchQuery_)) {
                    found = true;
                }
            }
            visible = visible && found;
        }

        // Column filters
        for (auto it = filters_.constBegin(); it != filters_.constEnd() && visible; ++it) {
            int col = -1;
            for (int c = 0; c < columns_.size(); ++c) {
                if (columns_[c].key == it.key()) { col = c; break; }
            }
            if (col >= 0) {
                auto* item = this->item(r, col);
                visible = visible && item && item->text().contains(it.value());
            }
        }

        setRowHidden(r, !visible);
    }
}

void AdvancedTableWidget::setupHeaderMenu() {
    connect(horizontalHeader(), &QHeaderView::customContextMenuRequested, this,
            [this](const QPoint& pos) {
        auto* menu = new QMenu(this);
        for (int i = 0; i < columns_.size(); ++i) {
            auto* action = menu->addAction(columns_[i].label);
            action->setCheckable(true);
            action->setChecked(columns_[i].visible);
            connect(action, &QAction::toggled, this, [this, i](bool visible) {
                columns_[i].visible = visible;
                if (visible) showColumn(i);
                else hideColumn(i);
            });
        }
        menu->exec(mapToGlobal(pos));
        menu->deleteLater();
    });
}

void AdvancedTableWidget::contextMenuEvent(QContextMenuEvent* event) {
    int row = rowAt(event->pos().y());
    if (row < 0) return;

    int id = rowIdAt(row);
    auto* menu = new QMenu(this);

    auto* copyAction = menu->addAction("Copy Row");
    connect(copyAction, &QAction::triggered, this, [this, row]() {
        QStringList cells;
        for (int c = 0; c < columnCount(); ++c) {
            auto* item = this->item(row, c);
            cells << (item ? item->text() : "");
        }
        QApplication::clipboard()->setText(cells.join("\t"));
    });

    menu->addSeparator();

    auto* exportAction = menu->addAction("Copy All Visible");
    connect(exportAction, &QAction::triggered, this, &AdvancedTableWidget::exportToClipboard);

    menu->exec(event->globalPos());
    menu->deleteLater();

    emit rowContextMenu(id, event->globalPos());
}

void AdvancedTableWidget::mouseDoubleClickEvent(QMouseEvent* event) {
    QTableWidget::mouseDoubleClickEvent(event);
}
