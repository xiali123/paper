#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QSplitter>
#include <QLabel>
#include <QList>
#include <QPair>

struct DiffLine {
    enum Type { Unchanged, Added, Removed, Modified };
    Type type{Unchanged};
    QString leftText;
    QString rightText;
    int leftLineNum{-1};
    int rightLineNum{-1};
};

class SideBySideDiff : public QWidget {
    Q_OBJECT

public:
    explicit SideBySideDiff(QWidget* parent = nullptr);

    void setContents(const QString& leftTitle, const QString& leftContent,
                     const QString& rightTitle, const QString& rightContent);
    void clear();

    int addedCount() const { return added_; }
    int removedCount() const { return removed_; }
    int modifiedCount() const { return modified_; }

signals:
    void diffComputed(int added, int removed, int modified);

private slots:
    void onSyncScroll(int value);

private:
    void setupUI();
    void computeDiff(const QStringList& left, const QStringList& right);
    void highlightDiff();

    QPlainTextEdit* leftEdit_{nullptr};
    QPlainTextEdit* rightEdit_{nullptr};
    QLabel* leftTitle_{nullptr};
    QLabel* rightTitle_{nullptr};
    QLabel* statsLabel_{nullptr};
    QSplitter* splitter_{nullptr};

    QList<DiffLine> diffLines_;
    int added_{0};
    int removed_{0};
    int modified_{0};
    bool syncingScroll_{false};
};
