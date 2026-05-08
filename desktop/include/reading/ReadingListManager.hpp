#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QList>
#include <QJsonObject>

struct ReadingList {
    int id{0};
    QString name;
    QString description;
    QString color;       // hex color
    int paperCount{0};
    QString createdAt;

    static ReadingList fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

class ApiManager;

class ReadingListManager : public QWidget {
    Q_OBJECT

public:
    explicit ReadingListManager(ApiManager* apiManager, QWidget* parent = nullptr);

signals:
    void paperAddedToList(int listId, int paperId);
    void openPaperRequested(int paperId);

private slots:
    void onCreateList();
    void onEditList();
    void onDeleteList();
    void onListSelected(QTreeWidgetItem* item, int col);
    void onRemovePaper();
    void onOpenPaper();
    void onAddPaperToList(int listId, int paperId);

private:
    void setupUI();
    void loadLists();
    void refreshTree();

    ApiManager* apiManager_{nullptr};

    QTreeWidget* listTree_{nullptr};
    QListWidget* papersWidget_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QLabel* countLabel_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* editBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* removePaperBtn_{nullptr};

    QList<ReadingList> lists_;
    int selectedListId_{-1};
};
