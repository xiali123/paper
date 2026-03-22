#include "PaperCardView.hpp"
#include <QEvent>
#include <QHBoxLayout>
#include <QComboBox>
#include <QGraphicsDropShadowEffect>

PaperCardView::PaperCardView(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupStyles();
}

void PaperCardView::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 20, 40, 20);
    mainLayout->setSpacing(12);

    // Scroll area for cards
    scrollArea_ = new QScrollArea(this);
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setFrameShape(QFrame::NoFrame);
    scrollArea_->setObjectName("paperScrollArea");
    scrollArea_->setMinimumHeight(500);  // More space for content

    // Scroll content
    scrollContent_ = new QWidget();
    cardsLayout_ = new QVBoxLayout(scrollContent_);
    cardsLayout_->setContentsMargins(20, 20, 20, 20);
    cardsLayout_->setSpacing(15);
    cardsLayout_->setAlignment(Qt::AlignTop);

    scrollArea_->setWidget(scrollContent_);

    // Empty state label
    emptyStateLabel_ = new QLabel("📭\n\n未找到相关论文\n\n请尝试其他关键词或调整搜索条件", this);
    emptyStateLabel_->setObjectName("emptyState");
    emptyStateLabel_->setAlignment(Qt::AlignCenter);
    emptyStateLabel_->setVisible(false);

    // Pagination bar
    paginationBar_ = new QWidget(this);
    paginationBar_->setObjectName("paginationBar");
    paginationBar_->setVisible(false);

    auto* paginationLayout = new QHBoxLayout(paginationBar_);
    paginationLayout->setContentsMargins(0, 10, 0, 10);
    paginationLayout->setSpacing(10);

    // Page size selector
    auto* pageSizeLabel = new QLabel("每页", paginationBar_);
    pageSizeLabel->setObjectName("paginationLabel");

    pageSizeCombo_ = new QComboBox(paginationBar_);
    pageSizeCombo_->setObjectName("pageSizeCombo");
    pageSizeCombo_->addItem("10", 10);
    pageSizeCombo_->addItem("20", 20);
    pageSizeCombo_->addItem("50", 50);
    pageSizeCombo_->addItem("100", 100);
    pageSizeCombo_->setCurrentIndex(1);  // Default to 20
    pageSizeCombo_->setCursor(Qt::PointingHandCursor);
    connect(pageSizeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCardView::onPageSizeChanged);

    auto* pageSizeUnitLabel = new QLabel("条", paginationBar_);
    pageSizeUnitLabel->setObjectName("paginationLabel");

    // Page info
    pageInfoLabel_ = new QLabel("1 / 1", paginationBar_);
    pageInfoLabel_->setObjectName("pageInfoLabel");

    paginationLayout->addWidget(pageSizeLabel);
    paginationLayout->addWidget(pageSizeCombo_);
    paginationLayout->addWidget(pageSizeUnitLabel);
    paginationLayout->addSpacing(30);
    paginationLayout->addWidget(pageInfoLabel_);
    paginationLayout->addSpacing(20);

    // Pagination buttons
    firstPageBtn_ = new QPushButton("⏮", paginationBar_);
    firstPageBtn_->setObjectName("paginationBtn");
    firstPageBtn_->setToolTip("第一页");
    firstPageBtn_->setCursor(Qt::PointingHandCursor);
    firstPageBtn_->setEnabled(false);
    connect(firstPageBtn_, &QPushButton::clicked, this, &PaperCardView::onFirstPage);

    prevPageBtn_ = new QPushButton("◀", paginationBar_);
    prevPageBtn_->setObjectName("paginationBtn");
    prevPageBtn_->setToolTip("上一页");
    prevPageBtn_->setCursor(Qt::PointingHandCursor);
    prevPageBtn_->setEnabled(false);
    connect(prevPageBtn_, &QPushButton::clicked, this, &PaperCardView::onPrevPage);

    nextPageBtn_ = new QPushButton("▶", paginationBar_);
    nextPageBtn_->setObjectName("paginationBtn");
    nextPageBtn_->setToolTip("下一页");
    nextPageBtn_->setCursor(Qt::PointingHandCursor);
    nextPageBtn_->setEnabled(false);
    connect(nextPageBtn_, &QPushButton::clicked, this, &PaperCardView::onNextPage);

    lastPageBtn_ = new QPushButton("⏭", paginationBar_);
    lastPageBtn_->setObjectName("paginationBtn");
    lastPageBtn_->setToolTip("最后一页");
    lastPageBtn_->setCursor(Qt::PointingHandCursor);
    lastPageBtn_->setEnabled(false);
    connect(lastPageBtn_, &QPushButton::clicked, this, &PaperCardView::onLastPage);

    paginationLayout->addWidget(firstPageBtn_);
    paginationLayout->addWidget(prevPageBtn_);
    paginationLayout->addWidget(nextPageBtn_);
    paginationLayout->addWidget(lastPageBtn_);

    // Load more button (legacy, keep for compatibility but not used with pagination)
    loadMoreButton_ = new QPushButton("加载更多", this);
    loadMoreButton_->setObjectName("loadMoreButton");
    loadMoreButton_->setVisible(false);
    loadMoreButton_->setCursor(Qt::PointingHandCursor);
    // Not connected - pagination is used instead

    mainLayout->addWidget(scrollArea_);
    mainLayout->addWidget(emptyStateLabel_);
    mainLayout->addWidget(paginationBar_);
    mainLayout->addWidget(loadMoreButton_);
}

void PaperCardView::setupStyles() {
    setStyleSheet(
        "PaperCardView {"
        "  background: transparent;"
        "}"
        "QWidget#paginationBar {"
        "  background: rgba(255, 255, 255, 0.95);"
        "  border-radius: 12px;"
        "  padding: 12px 24px;"
        "}"
        "QLabel#paginationLabel {"
        "  color: #6b7280;"
        "  font-size: 9pt;"
        "  padding: 6px 8px;"
        "  font-weight: 500;"
        "}"
        "QLabel#pageInfoLabel {"
        "  color: #4b5563;"
        "  font-size: 9pt;"
        "  padding: 6px 12px;"
        "  font-weight: 600;"
        "}"
        "QComboBox#pageSizeCombo {"
        "  background: white;"
        "  border: 1px solid #e5e7eb;"
        "  border-radius: 8px;"
        "  padding: 6px 16px;"
        "  min-width: 90px;"
        "  max-width: 110px;"
        "  font-size: 9pt;"
        "  font-weight: 500;"
        "}"
        "QComboBox#pageSizeCombo:hover {"
        "  border: 1px solid #6366f1;"
        "  background: #f9fafb;"
        "}"
        "QComboBox#pageSizeCombo::drop-down {"
        "  border: none;"
        "  width: 20px;"
        "}"
        "QPushButton#paginationBtn {"
        "  background: white;"
        "  border: 1px solid #e5e7eb;"
        "  border-radius: 8px;"
        "  padding: 6px 10px;"
        "  min-width: 32px;"
        "  max-width: 32px;"
        "  min-height: 32px;"
        "  max-height: 32px;"
        "  font-size: 10pt;"
        "  color: #4b5563;"
        "  font-weight: 600;"
        "}"
        "QPushButton#paginationBtn:hover {"
        "  background: #f9fafb;"
        "  border: 1px solid #6366f1;"
        "  color: #6366f1;"
        "}"
        "QPushButton#paginationBtn:disabled {"
        "  background: #f9fafb;"
        "  color: #d1d5db;"
        "  border: 1px solid #f3f4f6;"
        "  font-weight: 400;"
        "}"
    );
}

void PaperCardView::setPapers(const QList<Paper>& papers, int total, int currentPage) {
    qDebug() << "=== setPapers called ===";
    qDebug() << "Papers received:" << papers.count();
    qDebug() << "Total from server:" << total;
    qDebug() << "Current page parameter:" << currentPage;

    // Clear only the paper cards, NOT the pagination state
    clearPapersOnly();

    papers_ = papers;
    totalCount_ = (total >= 0) ? total : papers.count();

    // Update current page if provided (usually 1 for new searches)
    if (currentPage >= 1) {
        currentPage_ = currentPage;
        currentOffset_ = (currentPage_ - 1) * currentPageSize_;
    }

    qDebug() << "totalCount_ set to:" << totalCount_;
    qDebug() << "currentPageSize_:" << currentPageSize_;
    qDebug() << "currentPage_:" << currentPage_;
    qDebug() << "currentOffset_:" << currentOffset_;

    if (papers.isEmpty()) {
        emptyStateLabel_->setVisible(true);
        scrollArea_->setVisible(false);
        paginationBar_->setVisible(false);
        qDebug() << "No papers, showing empty state";
    } else {
        emptyStateLabel_->setVisible(false);
        scrollArea_->setVisible(true);

        // Display all papers for this page
        for (const auto& paper : papers) {
            addPaper(paper);
        }

        qDebug() << "Displayed" << papers.count() << "papers";

        // Show pagination if total papers > current page size
        if (totalCount_ > currentPageSize_) {
            paginationBar_->setVisible(true);
            updatePaginationControls();
            qDebug() << "Pagination shown";
        } else {
            paginationBar_->setVisible(false);
            qDebug() << "Pagination hidden (total=" << totalCount_ << "pageSize=" << currentPageSize_ << ")";
        }
    }
}

void PaperCardView::addPaper(const Paper& paper) {
    auto* card = createPaperCard(paper);
    cardsLayout_->addWidget(card);
}

void PaperCardView::clearPapersOnly() {
    // Clear only the paper cards, preserve pagination state
    while (cardsLayout_->count() > 0) {
        QLayoutItem* item = cardsLayout_->takeAt(0);
        if (item->widget()) {
            item->widget()->deleteLater();
        }
        delete item;
    }
    papers_.clear();
    // Note: We DON'T reset totalCount_, currentPage_, or currentOffset_
}

void PaperCardView::clear() {
    // Clear layout
    clearPapersOnly();

    // Reset all state
    totalCount_ = 0;
    currentPage_ = 1;
    currentOffset_ = 0;
    loadMoreButton_->setVisible(false);
}

QWidget* PaperCardView::createPaperCard(const Paper& paper) {
    auto* card = new QWidget();
    card->setObjectName("paperCard");
    card->setCursor(Qt::PointingHandCursor);
    card->setProperty("paperId", paper.id);
    card->setMaximumWidth(1200);  // Limit card width for better readability
    card->setMinimumHeight(120);  // Ensure minimum height for content

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

void PaperCardView::onPageSizeChanged(int index) {
    int newSize = pageSizeCombo_->itemData(index).toInt();
    qDebug() << "=== Page size changed ===";
    qDebug() << "Old size:" << currentPageSize_ << "New size:" << newSize;

    if (newSize != currentPageSize_) {
        currentPageSize_ = newSize;
        currentPage_ = 1;
        currentOffset_ = 0;

        qDebug() << "Emitting pageChanged with offset=" << currentOffset_ << "limit=" << currentPageSize_;
        emit pageChanged(currentOffset_, currentPageSize_);
    }
}

void PaperCardView::onFirstPage() {
    qDebug() << "=== First page clicked ===";
    if (currentPage_ != 1) {
        currentPage_ = 1;
        currentOffset_ = 0;
        qDebug() << "Emitting pageChanged with offset=" << currentOffset_ << "limit=" << currentPageSize_;
        emit pageChanged(currentOffset_, currentPageSize_);
    }
}

void PaperCardView::onPrevPage() {
    qDebug() << "=== Previous page clicked ===";
    if (currentPage_ > 1) {
        currentPage_--;
        currentOffset_ = (currentPage_ - 1) * currentPageSize_;
        qDebug() << "Emitting pageChanged with offset=" << currentOffset_ << "limit=" << currentPageSize_;
        emit pageChanged(currentOffset_, currentPageSize_);
    }
}

void PaperCardView::onNextPage() {
    qDebug() << "=== Next page clicked ===";
    int totalPages = (totalCount_ + currentPageSize_ - 1) / currentPageSize_;
    qDebug() << "Current page:" << currentPage_ << "Total pages:" << totalPages;
    if (currentPage_ < totalPages) {
        currentPage_++;
        currentOffset_ = (currentPage_ - 1) * currentPageSize_;
        qDebug() << "Emitting pageChanged with offset=" << currentOffset_ << "limit=" << currentPageSize_;
        emit pageChanged(currentOffset_, currentPageSize_);
    }
}

void PaperCardView::onLastPage() {
    qDebug() << "=== Last page clicked ===";
    int totalPages = (totalCount_ + currentPageSize_ - 1) / currentPageSize_;
    if (currentPage_ != totalPages) {
        currentPage_ = totalPages;
        currentOffset_ = (currentPage_ - 1) * currentPageSize_;
        qDebug() << "Emitting pageChanged with offset=" << currentOffset_ << "limit=" << currentPageSize_;
        emit pageChanged(currentOffset_, currentPageSize_);
    }
}

void PaperCardView::updatePaginationControls() {
    int totalPages = (totalCount_ + currentPageSize_ - 1) / currentPageSize_;

    // Update page info - more compact format
    pageInfoLabel_->setText(QString("%1 / %2")
                           .arg(currentPage_)
                           .arg(totalPages));

    // Update tooltip with full info
    pageInfoLabel_->setToolTip(QString("共 %1 篇论文").arg(totalCount_));

    // Update button states
    firstPageBtn_->setEnabled(currentPage_ > 1);
    prevPageBtn_->setEnabled(currentPage_ > 1);
    nextPageBtn_->setEnabled(currentPage_ < totalPages);
    lastPageBtn_->setEnabled(currentPage_ < totalPages);
}
