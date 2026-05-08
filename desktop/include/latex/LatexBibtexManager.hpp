#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QList>

struct BibtexEntry {
    QString type;       // article, book, inproceedings, etc.
    QString key;        // citation key
    QMap<QString, QString> fields; // author, title, year, journal, etc.

    static BibtexEntry fromString(const QString& text);
    QString toString() const;
};

class LatexBibtexManager : public QWidget {
    Q_OBJECT

public:
    explicit LatexBibtexManager(QWidget* parent = nullptr);

    void loadBibtex(const QString& content);
    QString exportBibtex() const;

signals:
    void insertCitation(const QString& key);
    void bibtexChanged(const QString& content);

private slots:
    void onAddEntry();
    void onEditEntry();
    void onDeleteEntry();
    void onImport();
    void onExport();
    void onInsertCite();
    void onSearch(const QString& text);
    void onEntrySelected(int row, int col);

private:
    void setupUI();
    void refreshTable();
    void showEditDialog(BibtexEntry& entry);

    QLineEdit* searchEdit_{nullptr};
    QTableWidget* entryTable_{nullptr};
    QTextEdit* previewEdit_{nullptr};
    QLabel* countLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* editBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* importBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* citeBtn_{nullptr};

    QList<BibtexEntry> entries_;
    int selectedRow_{-1};
};
