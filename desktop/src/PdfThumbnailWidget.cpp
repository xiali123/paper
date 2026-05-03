#include "PdfThumbnailWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidgetItem>
#include <QPainter>
#include <QPageLayout>

PdfThumbnailWidget::PdfThumbnailWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PdfThumbnailWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Navigation bar
    auto* navRow = new QHBoxLayout();

    prevBtn_ = new QPushButton("<");
    prevBtn_->setFixedSize(28, 28);
    prevBtn_->setStyleSheet("QPushButton { border-radius: 14px; font-weight: bold; }");
    connect(prevBtn_, &QPushButton::clicked, this, &PdfThumbnailWidget::onPrevPage);
    navRow->addWidget(prevBtn_);

    pageLabel_ = new QLabel("0 / 0");
    pageLabel_->setAlignment(Qt::AlignCenter);
    pageLabel_->setStyleSheet("font-size: 11px; font-weight: bold;");
    navRow->addWidget(pageLabel_, 1);

    nextBtn_ = new QPushButton(">");
    nextBtn_->setFixedSize(28, 28);
    nextBtn_->setStyleSheet("QPushButton { border-radius: 14px; font-weight: bold; }");
    connect(nextBtn_, &QPushButton::clicked, this, &PdfThumbnailWidget::onNextPage);
    navRow->addWidget(nextBtn_);

    layout->addLayout(navRow);

    // Thumbnail list
    thumbList_ = new QListWidget();
    thumbList_->setViewMode(QListWidget::IconMode);
    thumbList_->setIconSize(QSize(thumbWidth_, thumbWidth_ * 1.4));
    thumbList_->setSpacing(4);
    thumbList_->setResizeMode(QListWidget::Adjust);
    thumbList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; background: #1e293b; }"
        "QListWidget::item { padding: 4px; border-radius: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; }"
    );
    connect(thumbList_, &QListWidget::itemClicked,
            this, &PdfThumbnailWidget::onItemClicked);
    layout->addWidget(thumbList_, 1);
}

void PdfThumbnailWidget::loadFromPdfPath(const QString& path) {
    pdfPath_ = path;
    // In real implementation, use QPdfDocument to render thumbnails
    // Here we generate placeholder thumbnails
    pageCount_ = 0;
    thumbList_->clear();
    // Placeholder: show empty state
    updateCurrentHighlight();
}

void PdfThumbnailWidget::setPageCount(int count) {
    pageCount_ = qMax(0, count);
    currentPage_ = qBound(1, currentPage_, qMax(1, pageCount_));
    renderThumbnails();
}

void PdfThumbnailWidget::setCurrentPage(int page) {
    currentPage_ = qBound(1, page, qMax(1, pageCount_));
    updateCurrentHighlight();
    pageLabel_->setText(QString("%1 / %2").arg(currentPage_).arg(pageCount_));

    // Scroll to current
    if (currentPage_ > 0 && currentPage_ <= thumbList_->count()) {
        thumbList_->setCurrentRow(currentPage_ - 1);
    }
}

void PdfThumbnailWidget::setThumbnailSize(int width) {
    thumbWidth_ = qBound(60, width, 300);
    thumbList_->setIconSize(QSize(thumbWidth_, thumbWidth_ * 1.4));
}

void PdfThumbnailWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int page = thumbList_->row(item) + 1;
    currentPage_ = page;
    updateCurrentHighlight();
    pageLabel_->setText(QString("%1 / %2").arg(currentPage_).arg(pageCount_));
    emit pageClicked(page);
}

void PdfThumbnailWidget::onPrevPage() {
    if (currentPage_ > 1) setCurrentPage(currentPage_ - 1);
}

void PdfThumbnailWidget::onNextPage() {
    if (currentPage_ < pageCount_) setCurrentPage(currentPage_ + 1);
}

void PdfThumbnailWidget::renderThumbnails() {
    thumbList_->clear();

    for (int i = 0; i < pageCount_; ++i) {
        int page = i + 1;

        // Generate placeholder thumbnail
        QPixmap pixmap(thumbWidth_, static_cast<int>(thumbWidth_ * 1.4));
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing);

        // Page background
        painter.fillRect(pixmap.rect(), Qt::white);
        painter.setPen(QPen(QColor(200, 200, 200), 1));
        painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));

        // Fake text lines
        painter.setPen(QColor(180, 180, 180));
        int margin = 8;
        int y = 14;
        for (int line = 0; line < 12; ++line) {
            int w = (line == 0) ? pixmap.width() * 2 / 3 :
                    (line % 3 == 0) ? pixmap.width() * 4 / 5 :
                    pixmap.width() - margin * 2;
            painter.drawRect(margin, y, w, 4);
            y += 8;
        }

        // Page number
        painter.setPen(QColor(100, 100, 100));
        painter.setFont(QFont("", 10, QFont::Bold));
        painter.drawText(pixmap.rect(), Qt::AlignBottom | Qt::AlignHCenter,
                         QString::number(page));

        auto* item = new QListWidgetItem(QIcon(pixmap), QString::number(page));
        item->setData(Qt::UserRole, page);
        item->setSizeHint(QSize(thumbWidth_ + 8, static_cast<int>(thumbWidth_ * 1.4) + 24));
        thumbList_->addItem(item);

        emit pageRendered(page);
    }

    updateCurrentHighlight();
    pageLabel_->setText(QString("%1 / %2").arg(currentPage_).arg(pageCount_));
}

void PdfThumbnailWidget::updateCurrentHighlight() {
    for (int i = 0; i < thumbList_->count(); ++i) {
        auto* item = thumbList_->item(i);
        int page = item->data(Qt::UserRole).toInt();
        QFont font;
        font.setBold(page == currentPage_);
        item->setFont(font);
    }
}
