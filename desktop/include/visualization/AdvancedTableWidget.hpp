#pragma once

#include <QTableWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QLabel>
#include <QHeaderView>
#include <QMenu>

struct ColumnConfig {
    QString key;
    QString label;
    int width{-1};
    bool visible{true};
    bool sortable{true};
};

class AdvancedTableWidget : public QTableWidget {
    Q_OBJECT

public:
    explicit AdvancedTableWidget(QWidget* parent = nullptr);

    void configureColumns(const QList<ColumnConfig>& columns);
    void addRow(const QMap<QString, QString>& data, int rowId = -1);
    void updateRow(int row, const QMap<QString, QString>& data);
    void removeRow(int row);
    void removeRowById(int rowId);

    int findRowById(int rowId) const;
    int rowIdAt(int row) const;

    void setFilter(const QString& column, const QString& value);
    void clearFilters();
    void setSearchQuery(const QString& query);

    void sortyByColumn(const QString& key, Qt::SortOrder order = Qt::AscendingOrder);

    QList<int> visibleRowIds() const;
    int visibleRowCount() const;

    void exportToClipboard() const;

signals:
    void rowDoubleClicked(int rowId);
    void rowContextMenu(int rowId, const QPoint& pos);
    void filterChanged(const QMap<QString, QString>& filters);
    void selectionChanged(const QList<int>& ids);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onHeaderClicked(int logicalIndex);
    void onFilterChanged();

private:
    void applyFilters();
    void setupHeaderMenu();

    QList<ColumnConfig> columns_;
    QMap<QString, QString> filters_;
    QString searchQuery_;
    QMap<int, int> rowIdMap_; // rowId -> tableRow
    int nextRowId_{1};
    int sortColumn_{-1};
    Qt::SortOrder sortOrder_{Qt::AscendingOrder};

    QWidget* filterBar_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QMap<QString, QComboBox*> filterCombos_;
};
