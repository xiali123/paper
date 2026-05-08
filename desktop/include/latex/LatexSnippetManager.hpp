#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QLineEdit>
#include <QMap>

struct LatexSnippet {
    QString name;
    QString category;
    QString insertBefore;
    QString insertAfter;
    QString description;
};

class LatexSnippetManager : public QWidget {
    Q_OBJECT

public:
    explicit LatexSnippetManager(QWidget* parent = nullptr);

signals:
    void snippetInsert(const QString& before, const QString& after);

private slots:
    void onSearch(const QString& text);
    void onItemActivated(QTreeWidgetItem* item, int column);

private:
    void setupUI();
    void loadSnippets();
    void addCategory(const QString& name, const QString& icon, const QList<LatexSnippet>& snippets);

    QLineEdit* searchEdit_{nullptr};
    QTreeWidget* snippetTree_{nullptr};
    QMap<QString, QList<LatexSnippet>> allSnippets_;
};
