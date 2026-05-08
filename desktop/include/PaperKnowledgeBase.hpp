#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QList>
#include <QMap>
#include <QSettings>

struct KBEntry {
    int id{-1};
    QString title;
    QString category;
    QString content;
    QStringList tags;
    QStringList relatedPapers;
    qint64 createdAt{0};
    qint64 updatedAt{0};
};

class PaperKnowledgeBase : public QWidget {
    Q_OBJECT

public:
    explicit PaperKnowledgeBase(QWidget* parent = nullptr);

    void addEntry(const KBEntry& entry);
    void updateEntry(int entryId, const KBEntry& data);
    void removeEntry(int entryId);
    QList<KBEntry> entries() const;
    QList<KBEntry> search(const QString& query) const;
    QList<KBEntry> byCategory(const QString& category) const;

signals:
    void entryAdded(int id);
    void entryUpdated(int id);
    void entryRemoved(int id);

private slots:
    void onAdd();
    void onEdit();
    void onDelete();
    void onSearch();
    void onCategoryChanged(int index);
    void onEntrySelected();
    void onExport();
    void onImport();

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QTreeWidget* entryTree_{nullptr};
    QTextEdit* contentEdit_{nullptr};
    QLineEdit* titleEdit_{nullptr};
    QLineEdit* tagsEdit_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QComboBox* categoryCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* editBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* searchBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* importBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<KBEntry> entries_;
    int nextId_{1};
    int selectedId_{-1};
};
