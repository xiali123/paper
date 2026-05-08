#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QList>

class PaperCardView;
struct Paper;

class BatchOperationsBar : public QWidget {
    Q_OBJECT

public:
    explicit BatchOperationsBar(QWidget* parent = nullptr);

    void setSelection(const QList<int>& paperIds);
    void clearSelection();
    int selectedCount() const { return selectedIds_.size(); }

signals:
    void batchFavorite(const QList<int>& ids, bool favorite);
    void batchExport(const QList<int>& ids);
    void batchDelete(const QList<int>& ids);
    void batchAddTags(const QList<int>& ids, const QStringList& tags);
    void selectionCleared();

private slots:
    void onFavorite();
    void onUnfavorite();
    void onExport();
    void onDelete();
    void onAddTags();
    void onClear();

private:
    void setupUI();

    QLabel* countLabel_{nullptr};
    QPushButton* favBtn_{nullptr};
    QPushButton* unfavBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};
    QPushButton* tagBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};

    QList<int> selectedIds_;
};
