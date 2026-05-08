#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QMap>

struct JournalInfo {
    QString name;
    QString abbreviation;
    QString field;
    double impactFactor{0.0};
    int paperCount{0};
    QString publisher;
    QString issn;
};

class JournalBrowserWidget : public QWidget {
    Q_OBJECT

public:
    explicit JournalBrowserWidget(QWidget* parent = nullptr);

    void setJournals(const QList<JournalInfo>& journals);
    QList<JournalInfo> journals() const;

    void setFavorites(const QStringList& issns);
    QStringList favorites() const;

signals:
    void journalSelected(const QString& issn);
    void searchPapersInJournal(const QString& journalName);
    void favoriteToggled(const QString& issn, bool favorite);

private slots:
    void onSearchChanged(const QString& text);
    void onSortChanged(int index);
    void onJournalClicked(QTreeWidgetItem* item, int column);
    void onToggleFavorite();
    void onSearchPapers();

private:
    void setupUI();
    void refreshTree();
    void updateStats();

    QTreeWidget* journalTree_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QComboBox* sortCombo_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* favBtn_{nullptr};
    QPushButton* searchBtn_{nullptr};

    QList<JournalInfo> journals_;
    QSet<QString> favoriteIssns_;
    QString selectedIssn_;
};
