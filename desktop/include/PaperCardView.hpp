#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>
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
 * - Load more button
 */
class PaperCardView : public QWidget {
    Q_OBJECT

public:
    explicit PaperCardView(QWidget* parent = nullptr);
    void setPapers(const QList<Paper>& papers);
    void addPaper(const Paper& paper);
    void clear();
    int paperCount() const { return papers_.count(); }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void paperSelected(int paperId);

private slots:
    void onCardClicked();
    void onLoadMoreClicked();

private:
    void setupUI();
    void setupStyles();
    QWidget* createPaperCard(const Paper& paper);
    QString getLevelStyle(const QString& level) const;
    QString formatAuthors(const QString& authors, int maxCount = 2) const;

    QScrollArea* scrollArea_{nullptr};
    QWidget* scrollContent_{nullptr};
    QVBoxLayout* cardsLayout_{nullptr};
    QPushButton* loadMoreButton_{nullptr};
    QLabel* emptyStateLabel_{nullptr};

    QList<Paper> papers_;
    static constexpr int BATCH_SIZE = 10;
    int displayedCount_{0};
};
