#pragma once

#include <QWidget>
#include <QTreeWidget>
#include <QMap>

class QPlainTextEdit;

struct OutlineItem {
    int level;          // 1=part, 2=chapter, 3=section, 4=subsection, 5=subsubsection
    QString title;
    int line;           // line number in document
};

class LatexDocumentOutline : public QWidget {
    Q_OBJECT

public:
    explicit LatexDocumentOutline(QWidget* parent = nullptr);

    void parseDocument(const QString& content);
    void clear();

signals:
    void navigateToLine(int line);

private slots:
    void onItemClicked(QTreeWidgetItem* item, int column);

private:
    void setupUI();

    QTreeWidget* outlineTree_{nullptr};
    QLabel* emptyLabel_{nullptr};
    QList<OutlineItem> items_;
};
