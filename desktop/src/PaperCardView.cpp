#include "PaperCardView.hpp"
#include <QEvent>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

PaperCardView::PaperCardView(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupStyles();
}

void PaperCardView::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 20, 40, 40);
    mainLayout->setSpacing(0);

    // Scroll area for cards
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setObjectName("paperScrollArea");

    // Scroll content
    scrollContent_ = new QWidget();
    cardsLayout_ = new QVBoxLayout(scrollContent_);
    cardsLayout_->setContentsMargins(0, 0, 0, 0);
    cardsLayout_->setSpacing(0);
    cardsLayout_->setAlignment(Qt::AlignTop);

    scrollArea_->setWidget(scrollContent_);

    // Empty state label
    emptyStateLabel_ = new QLabel("📭\n\n未找到相关论文\n\n请尝试其他关键词或调整搜索条件", this);
    emptyStateLabel_->setObjectName("emptyState");
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setVisible(false);

    // Load more button
    loadMoreButton_ = new QPushButton("加载更多", this);
    loadMoreButton_->setObjectName("loadMoreButton");
    loadMoreButton_->setVisible(false);
    loadMoreButton_->setCursor(Qt::PointingHandCursor);
    connect(loadMoreButton_, &QPushButton::clicked, this, &PaperCardView::onLoadMoreClicked);

    mainLayout->addWidget(scrollArea_);
    mainLayout->addWidget(emptyStateLabel_);
    mainLayout->addWidget(loadMoreButton_);
}

void PaperCardView::setupStyles() {
    setStyleSheet(
        "PaperCardView {"
        "  background: transparent;"
        "}"
    );
}

void PaperCardView::setPapers(const QList<Paper>& papers) {
    clear();
    papers_ = papers;
    displayedCount_ = 0;

    if (papers.isEmpty()) {
        emptyStateLabel_->setVisible(true);
        scrollArea_->setVisible(false);
    } else {
        emptyStateLabel_->setVisible(false);
        scrollArea_->setVisible(true);

        // Show first batch
        int showCount = std::min(static_cast<qsizetype>(BATCH_SIZE), papers.count());
        for (int i = 0; i < showCount; ++i) {
            addPaper(papers[i]);
        }
        displayedCount_ = showCount;

        // Show load more button if there are more papers
        if (displayedCount_ < papers.count()) {
            loadMoreButton_->setText(QString("加载更多 (剩余 %1 篇)").arg(papers.count() - displayedCount_));
            loadMoreButton_->setVisible(true);
        }
    }
}

void PaperCardView::addPaper(const Paper& paper) {
    auto* card = createPaperCard(paper);
    cardsLayout_->addWidget(card);
}

void PaperCardView::clear() {
    // Clear layout
    while (cardsLayout_->count() > 0) {
        QLayoutItem* item = cardsLayout_->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    papers_.clear();
    displayedCount_ = 0;
    loadMoreButton_->setVisible(false);
}

QWidget* PaperCardView::createPaperCard(const Paper& paper) {
    auto* card = new QWidget();
    card->setObjectName("paperCard");
    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("paperId", paper.id);

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(30, 20, 30, 20);
    layout->setSpacing(15);

    // Header: Title + Level badge
    auto* headerLayout = new QHBoxLayout();
    headerLayout->setSpacing(15);

    auto* titleLabel = new QLabel(paper.title, card);
    titleLabel->setObjectName("paperTitle");
    titleLabel->setWordWrap(true);
    titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* levelBadge = new QLabel(paper.level.toUpper(), card);
    levelBadge->setObjectName("levelBadge");
    levelBadge->setProperty("level", paper.level.toLower());

    headerLayout->addWidget(titleLabel);
    headerLayout->addWidget(levelBadge, 0, Qt::AlignTop);

    // Meta info: Journal, Year, Authors
    auto* metaLayout = new QHBoxLayout();
    metaLayout->setSpacing(15);

    auto* journalLabel = new QLabel(paper.journal, card);
    journalLabel->setObjectName("paperMeta");
    journalLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

    auto* yearLabel = new QLabel(paper.year, card);
    yearLabel->setObjectName("paperMeta");

    auto* authorsLabel = new QLabel(formatAuthors(paper.authors), card);
    authorsLabel->setObjectName("paperAuthors");
    authorsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    metaLayout->addWidget(journalLabel);
    metaLayout->addWidget(yearLabel);
    metaLayout->addWidget(authorsLabel);
    metaLayout->addStretch();

    // DOI link if available
    if (!paper.doiUrl.isEmpty()) {
        auto* linkLabel = new QLabel(QString("<a href=\"%1\">DOI</a>").arg(paper.doiUrl), card);
        linkLabel->setObjectName("paperLink");
        linkLabel->setOpenExternalLinks(true);
        layout->addWidget(linkLabel);
    }

    // Add to main layout
    layout->addLayout(headerLayout);
    layout->addLayout(metaLayout);

    // Card styling
    card->setStyleSheet(
        "QWidget#paperCard {"
        "  background-color: rgba(255, 255, 255, 0.95);"
        "  border-left: 4px solid #667eea;"
        "  border-bottom: 1px solid #e5e7eb;"
        "  padding: 5px;"
        "}"
        "QWidget#paperCard:hover {"
        "  background-color: #f9fafb;"
        "  border-left: 5px solid #667eea;"
        "}"
    );

    titleLabel->setStyleSheet(
        "QLabel#paperTitle {"
        "  color: #1f2937;"
        "  font-size: 12pt;"
        "  font-weight: 600;"
        "  background: transparent;"
        "}"
    );

    levelBadge->setStyleSheet(getLevelStyle(paper.level));

    journalLabel->setStyleSheet(
        "QLabel#paperMeta {"
        "  color: #6b7280;"
        "  font-size: 9pt;"
        "  background: transparent;"
        "}"
    );

    yearLabel->setStyleSheet(
        "QLabel#paperMeta {"
        "  color: #6b7280;"
        "  font-size: 9pt;"
        "  background: transparent;"
        "}"
    );

    authorsLabel->setStyleSheet(
        "QLabel#paperAuthors {"
        "  color: #9ca3af;"
        "  font-size: 9pt;"
        "  background: transparent;"
        "}"
    );

    // Shadow effect
    auto* shadowEffect = new QGraphicsDropShadowEffect(card);
    shadowEffect->setBlurRadius(0);
    shadowEffect->setColor(QColor(0, 0, 0, 10));
    shadowEffect->setOffset(0, 2);
    card->setGraphicsEffect(shadowEffect);

    // Connect click event - override mousePressEvent instead
    card->installEventFilter(this);

    return card;
}

QString PaperCardView::getLevelStyle(const QString& level) const {
    QString baseStyle = "QLabel#levelBadge {"
                       "  padding: 6px 14px;"
                       "  border-radius: 20px;"
                       "  font-size: 9pt;"
                       "  font-weight: 600;"
                       "}";

    if (level.toUpper() == "A") {
        return baseStyle +
               "color: #991b1b;"
               "background-color: #fecaca;";
    } else if (level.toUpper() == "B") {
        return baseStyle +
               "color: #9a3412;"
               "background-color: #fed7aa;";
    } else if (level.toUpper() == "C") {
        return baseStyle +
               "color: #374151;"
               "background-color: #d1d5db;";
    }
    return baseStyle +
           "color: #6b7280;"
           "background-color: #f3f4f6;";
}

QString PaperCardView::formatAuthors(const QString& authors, int maxCount) const {
    QStringList authorList = authors.split(",", Qt::SkipEmptyParts);
    if (authorList.isEmpty()) return "";

    QString formatted;
    int count = std::min(static_cast<qsizetype>(maxCount), authorList.count());
    for (int i = 0; i < count; ++i) {
        if (i > 0) formatted += ", ";
        formatted += authorList[i].trimmed();
    }

    if (authorList.count() > maxCount) {
        formatted += " et al.";
    }

    return formatted;
}

void PaperCardView::onCardClicked() {
    // This will be called through event filter
}

bool PaperCardView::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget* card = qobject_cast<QWidget*>(obj);
        if (card) {
            QVariant paperId = card->property("paperId");
            if (paperId.isValid()) {
                emit paperSelected(paperId.toInt());
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void PaperCardView::onLoadMoreClicked() {
    int remaining = papers_.count() - displayedCount_;
    int loadCount = std::min(remaining, BATCH_SIZE);

    for (int i = 0; i < loadCount; ++i) {
        addPaper(papers_[displayedCount_]);
        displayedCount_++;
    }

    // Update button text or hide
    if (displayedCount_ >= papers_.count()) {
        loadMoreButton_->setVisible(false);
    } else {
        loadMoreButton_->setText(QString("加载更多 (剩余 %1 篇)").arg(papers_.count() - displayedCount_));
    }
}
