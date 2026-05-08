#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct Comment {
    int id{-1};
    int paperId{-1};
    int parentId{-1};
    QString author;
    QString text;
    QString category;
    qint64 timestamp{0};
    bool resolved{false};
};

class PaperCommentWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperCommentWidget(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title);
    void addComment(const Comment& comment);
    void addReply(int parentId, const QString& text, const QString& author = "Me");
    void resolveComment(int commentId);
    void deleteComment(int commentId);
    QList<Comment> comments(int paperId) const;
    int unresolvedCount(int paperId) const;

signals:
    void commentAdded(int paperId, int commentId);
    void commentResolved(int paperId, int commentId);
    void commentDeleted(int commentId);

private slots:
    void onAdd();
    void onReply();
    void onResolve();
    void onDelete();
    void onFilterChanged(int index);
    void onItemClicked(QTreeWidgetItem* item, int col);

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshTree();
    void buildTree(QTreeWidgetItem* parent, int parentId);
    void updateStats();

    QTreeWidget* tree_{nullptr};
    QTextEdit* inputEdit_{nullptr};
    QLineEdit* authorEdit_{nullptr};
    QComboBox* categoryCombo_{nullptr};
    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* replyBtn_{nullptr};
    QPushButton* resolveBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QLabel* paperLabel_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<Comment> allComments_;
    int currentPaperId_{-1};
    int nextId_{1};
};
