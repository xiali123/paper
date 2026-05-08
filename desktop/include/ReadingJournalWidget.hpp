#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QDateEdit>
#include <QList>
#include <QSettings>

struct JournalEntry {
    int id{-1};
    QDate date;
    QString title;
    QString mood{"neutral"};
    QString content;
    QStringList tags;
    int paperId{-1};
    qint64 createdAt{0};
};

class ReadingJournalWidget : public QWidget {
    Q_OBJECT

public:
    explicit ReadingJournalWidget(QWidget* parent = nullptr);

    void addEntry(const JournalEntry& entry);
    void removeEntry(int id);
    QList<JournalEntry> entries() const;
    QList<JournalEntry> entriesByDate(const QDate& from, const QDate& to) const;
    QList<JournalEntry> searchEntries(const QString& query) const;

signals:
    void entryAdded(int id);
    void entryRemoved(int id);
    void entrySelected(int id, const QString& title);

private slots:
    void onAdd();
    void onDelete();
    void onSearch();
    void onFilterChanged(int index);
    void onEntrySelected();
    void onExport();
    void onToday();

private:
    void setupUI();
    void refreshTree();
    void updateStats();
    void loadSettings();
    void saveSettings();

    QTreeWidget* entryTree_{nullptr};
    QTextEdit* contentEdit_{nullptr};
    QLineEdit* titleEdit_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QDateEdit* dateEdit_{nullptr};
    QComboBox* moodCombo_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* searchBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* todayBtn_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<JournalEntry> entries_;
    int nextId_{1};
    int selectedId_{-1};
};
