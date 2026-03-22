#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>

/**
 * @brief Paper data structure
 */
struct Paper {
    int id;
    QString title;
    QString journal;
    QString year;
    QString level;
    QString authors;
    QString doiUrl;
};

/**
 * @brief Modern card-based paper list view matching web frontend
 *
 * Features:
 * - Card-based layout (not table)
 * - CCF Level badges with colors
 * - Hover effects
 * - Click to select
 * - Pagination with customizable page size
 */
class PaperCardView : public QWidget {
    Q_OBJECT

public:
    explicit PaperCardView(QWidget* parent = nullptr);
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
    void pageChanged(int offset, int limit);  // New signal for pagination

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
    void clearPapersOnly();  // Clear papers without resetting pagination state

    QScrollArea* scrollArea_{nullptr};
    QWidget* scrollContent_{nullptr};
    QVBoxLayout* cardsLayout_{nullptr};
    QPushButton* loadMoreButton_{nullptr};
    QLabel* emptyStateLabel_{nullptr};

    // Pagination controls
    class QComboBox* pageSizeCombo_{nullptr};
    class QLabel* pageInfoLabel_{nullptr};
    class QPushButton* firstPageBtn_{nullptr};
    class QPushButton* prevPageBtn_{nullptr};
    class QPushButton* nextPageBtn_{nullptr};
    class QPushButton* lastPageBtn_{nullptr};
    QWidget* paginationBar_{nullptr};

    QList<Paper> papers_;
    int totalCount_{0};           // Total papers from server
    int currentPage_{1};          // Current page number (1-based)
    int currentPageSize_{20};     // Current page size
    int currentOffset_{0};        // Current offset for API
};
