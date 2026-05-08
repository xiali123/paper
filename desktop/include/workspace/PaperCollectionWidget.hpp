#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QJsonObject>
#include <QJsonArray>

struct PaperCollection {
    int id{-1};
    QString name;
    QString description;
    QString color{"#3b82f6"};
    QStringList paperIds;
    qint64 createdAt{0};
    qint64 updatedAt{0};
};

class PaperCollectionWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperCollectionWidget(QWidget* parent = nullptr);

    void setCollections(const QList<PaperCollection>& collections);
    QList<PaperCollection> collections() const;

    void addCollection(const PaperCollection& collection);
    void removeCollection(int collectionId);
    void addPaperToCollection(int collectionId, int paperId);
    void removePaperFromCollection(int collectionId, int paperId);

signals:
    void collectionSelected(int collectionId);
    void collectionCreated(const PaperCollection& collection);
    void collectionDeleted(int collectionId);
    void paperAdded(int collectionId, int paperId);
    void paperRemoved(int collectionId, int paperId);

private slots:
    void onCreateCollection();
    void onDeleteCollection();
    void onEditCollection();
    void onCollectionItemClicked(QTreeWidgetItem* item, int column);
    void onPaperContextMenu(const QPoint& pos);

private:
    void setupUI();
    void refreshTree();
    void refreshPapers(int collectionId);

    QTreeWidget* collectionTree_{nullptr};
    QListWidget* papersList_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* editBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};

    QList<PaperCollection> collections_;
    int selectedCollectionId_{-1};
    int nextId_{1};
};
