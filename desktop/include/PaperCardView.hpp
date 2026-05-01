#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include "PaperTypes.hpp"

class FavoriteManager;

class PaperCardView : public QWidget {
    Q_OBJECT

public:
    explicit PaperCardView(QWidget* parent = nullptr);
    void setFavoriteManager(FavoriteManager* mgr);
    void setPapers(const QList<Paper>& papers, int total = -1, int currentPage = 1);
    void addPaper(const Paper& paper);
    void clear();
    int paperCount() const { return papers_.count(); }
    const QList<Paper>& getPapers() const { return papers_; }
    int getTotalCount() const { return totalCount_; }
    int getCurrentPage() const { return currentPage_; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void paperSelected(int paperId);
    void pageChanged(int offset, int limit);
    void favoriteToggled(int paperId, bool favorite);

private slots:
    void onCardClicked();
    void onPageSizeChanged(int index);
    void onFirstPage();
    void onPrevPage();
    void onNextPage();
    void onLastPage();

private:
    void setupUI();
    void setupStyles();
    QWidget* createPaperCard(const Paper& paper);
    QString getLevelStyle(const QString& level) const;
    QString formatAuthors(const QString& authors, int maxCount = 2) const;
    void updatePaginationControls();
    void clearPapersOnly();

    QScrollArea* scrollArea_{nullptr};
    QWidget* scrollContent_{nullptr};
    QVBoxLayout* cardsLayout_{nullptr};
    QPushButton* loadMoreButton_{nullptr};
    QLabel* emptyStateLabel_{nullptr};

    QComboBox* pageSizeCombo_{nullptr};
    QLabel* pageInfoLabel_{nullptr};
    QPushButton* firstPageBtn_{nullptr};
    QPushButton* prevPageBtn_{nullptr};
    QPushButton* nextPageBtn_{nullptr};
    QPushButton* lastPageBtn_{nullptr};
    QWidget* paginationBar_{nullptr};

    FavoriteManager* favoriteManager_{nullptr};
    QList<Paper> papers_;
    int totalCount_{0};
    int currentPage_{1};
    int currentPageSize_{20};
    int currentOffset_{0};
};
