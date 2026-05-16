#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QMap>
#include <QPair>

struct TagInfo {
    QString name;
    QString category;
    int count{0};
    QString color;
};

class TagManager : public QWidget {
    Q_OBJECT

public:
    explicit TagManager(QWidget* parent = nullptr);

    void setTags(const QList<TagInfo>& tags);
    QList<TagInfo> tags() const;

    void addTag(const TagInfo& tag);
    void removeTag(const QString& name);
    void renameTag(const QString& oldName, const QString& newName);
    void mergeTags(const QStringList& sources, const QString& target);
    void recolorTag(const QString& name, const QString& color);

    QStringList allTagNames() const;
    QStringList categories() const;

signals:
    void tagAdded(const TagInfo& tag);
    void tagRemoved(const QString& name);
    void tagRenamed(const QString& oldName, const QString& newName);
    void tagsMerged(const QStringList& sources, const QString& target);
    void tagSelected(const QString& name);

private slots:
    void onAddTag();
    void onDeleteTag();
    void onRenameTag();
    void onMergeTags();
    void onSearchChanged(const QString& text);
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    void setupUI();
    void refreshTree();
    QColor hashColor(const QString& str) const;

    QTreeWidget* tagTree_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QLineEdit* nameEdit_{nullptr};
    QComboBox* categoryCombo_{nullptr};
    QLabel* statsLabel_{nullptr};
    QPushButton* addBtn_{nullptr};

    QMap<QString, TagInfo> tags_;
    QString selectedTag_;
};
