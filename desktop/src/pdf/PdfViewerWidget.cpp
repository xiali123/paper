#include "pdf/PdfViewerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPdfDocument>
#include <QFileInfo>

PdfViewerWidget::PdfViewerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PdfViewerWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(8, 4, 8, 4);
    toolbar->setSpacing(4);

    // Navigation
    auto* firstBtn = new QPushButton("|<");
    firstBtn->setFixedSize(28, 28);
    connect(firstBtn, &QPushButton::clicked, this, &PdfViewerWidget::onFirstPage);
    toolbar->addWidget(firstBtn);

    prevBtn_ = new QPushButton("<");
    prevBtn_->setFixedSize(28, 28);
    connect(prevBtn_, &QPushButton::clicked, this, &PdfViewerWidget::onPrevPage);
    toolbar->addWidget(prevBtn_);

    pageInfo_ = new QLabel("0 / 0");
    pageInfo_->setStyleSheet("font-size: 11px; font-weight: bold; min-width: 60px;");
    pageInfo_->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(pageInfo_);

    nextBtn_ = new QPushButton(">");
    nextBtn_->setFixedSize(28, 28);
    connect(nextBtn_, &QPushButton::clicked, this, &PdfViewerWidget::onNextPage);
    toolbar->addWidget(nextBtn_);

    auto* lastBtn = new QPushButton(">|");
    lastBtn->setFixedSize(28, 28);
    connect(lastBtn, &QPushButton::clicked, this, &PdfViewerWidget::onLastPage);
    toolbar->addWidget(lastBtn);

    // Page slider
    pageSlider_ = new QSlider(Qt::Horizontal);
    pageSlider_->setRange(1, 1);
    pageSlider_->setFixedWidth(120);
    connect(pageSlider_, &QSlider::valueChanged, this, &PdfViewerWidget::onSliderMoved);
    toolbar->addWidget(pageSlider_);

    toolbar->addSpacing(16);

    // Zoom
    zoomOutBtn_ = new QPushButton("-");
    zoomOutBtn_->setFixedSize(28, 28);
    connect(zoomOutBtn_, &QPushButton::clicked, this, &PdfViewerWidget::onZoomOut);
    toolbar->addWidget(zoomOutBtn_);

    auto* zoomLabel = new QLabel("100%");
    zoomLabel->setObjectName("zoomLabel");
    zoomLabel->setStyleSheet("font-size: 11px; min-width: 40px;");
    zoomLabel->setAlignment(Qt::AlignCenter);
    toolbar->addWidget(zoomLabel);

    zoomInBtn_ = new QPushButton("+");
    zoomInBtn_->setFixedSize(28, 28);
    connect(zoomInBtn_, &QPushButton::clicked, this, &PdfViewerWidget::onZoomIn);
    toolbar->addWidget(zoomInBtn_);

    toolbar->addSpacing(8);

    // Fit mode
    fitCombo_ = new QComboBox();
    fitCombo_->addItems({"Fit Width", "Fit Page", "Custom"});
    fitCombo_->setFixedWidth(90);
    connect(fitCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PdfViewerWidget::onFitModeChanged);
    toolbar->addWidget(fitCombo_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    // Scroll area with page label
    scrollArea_ = new QScrollArea();
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setStyleSheet(
        "QScrollArea { background: #1e293b; border: none; }"
    );

    pageLabel_ = new QLabel();
    pageLabel_->setAlignment(Qt::AlignCenter);
    pageLabel_->setStyleSheet("background: white; margin: 20px;");
    scrollArea_->setWidget(pageLabel_);

    layout->addWidget(scrollArea_, 1);
}

void PdfViewerWidget::loadFile(const QString& path) {
    filePath_ = path;
    if (!QFileInfo::exists(path)) {
        pageLabel_->setText("File not found: " + path);
        return;
    }

    // In real implementation, use QPdfDocument
    // QPdfDocument doc;
    // doc.load(path);
    // pageCount_ = doc.pageCount();
    // For now, show placeholder
    pageLabel_->setText(QString("PDF: %1\n\n[PDF rendering placeholder]\n"
        "In production, this would use QPdfDocument to render pages.")
        .arg(QFileInfo(path).fileName()));
    pageCount_ = 0;
    currentPage_ = 1;
    updatePageInfo();
    updateNavButtons();
    emit fileLoaded(path);
}

void PdfViewerWidget::setPageCount(int count) {
    pageCount_ = qMax(0, count);
    pageSlider_->setRange(1, qMax(1, pageCount_));
    updatePageInfo();
    updateNavButtons();
}

void PdfViewerWidget::setCurrentPage(int page) {
    currentPage_ = qBound(1, page, qMax(1, pageCount_));
    pageSlider_->setValue(currentPage_);
    renderPage();
    updatePageInfo();
    updateNavButtons();
    emit pageChanged(currentPage_);
}

void PdfViewerWidget::setZoom(int percent) {
    zoomPercent_ = qBound(25, percent, 400);
    auto* zoomLabel = findChild<QLabel*>("zoomLabel");
    if (zoomLabel) zoomLabel->setText(QString("%1%").arg(zoomPercent_));
    renderPage();
    emit zoomChanged(zoomPercent_);
}

void PdfViewerWidget::setFitMode(const QString& mode) {
    fitMode_ = mode;
    int idx = (mode == "width") ? 0 : (mode == "page") ? 1 : 2;
    fitCombo_->setCurrentIndex(idx);
}

void PdfViewerWidget::onPrevPage() {
    if (currentPage_ > 1) setCurrentPage(currentPage_ - 1);
}

void PdfViewerWidget::onNextPage() {
    if (currentPage_ < pageCount_) setCurrentPage(currentPage_ + 1);
}

void PdfViewerWidget::onFirstPage() {
    setCurrentPage(1);
}

void PdfViewerWidget::onLastPage() {
    setCurrentPage(qMax(1, pageCount_));
}

void PdfViewerWidget::onZoomIn() {
    setZoom(zoomPercent_ + 25);
}

void PdfViewerWidget::onZoomOut() {
    setZoom(zoomPercent_ - 25);
}

void PdfViewerWidget::onFitModeChanged(int index) {
    switch (index) {
        case 0: fitMode_ = "width"; break;
        case 1: fitMode_ = "page"; break;
        case 2: fitMode_ = "custom"; break;
    }
    renderPage();
}

void PdfViewerWidget::onSliderMoved(int value) {
    currentPage_ = value;
    renderPage();
    updatePageInfo();
    emit pageChanged(currentPage_);
}

void PdfViewerWidget::renderPage() {
    // Placeholder rendering
    int w = static_cast<int>(612 * zoomPercent_ / 100.0);
    int h = static_cast<int>(792 * zoomPercent_ / 100.0);

    QPixmap pixmap(w, h);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(pixmap.rect(), Qt::white);
    painter.setPen(QPen(QColor(200, 200, 200), 1));
    painter.drawRect(pixmap.rect().adjusted(0, 0, -1, -1));

    painter.setPen(Qt::black);
    painter.setFont(QFont("", 12));
    painter.drawText(pixmap.rect(), Qt::AlignCenter,
        QString("Page %1 of %2\nZoom: %3%")
            .arg(currentPage_).arg(pageCount_).arg(zoomPercent_));

    pageLabel_->setPixmap(pixmap);
}

void PdfViewerWidget::updatePageInfo() {
    pageInfo_->setText(QString("%1 / %2").arg(currentPage_).arg(pageCount_));
}

void PdfViewerWidget::updateNavButtons() {
    prevBtn_->setEnabled(currentPage_ > 1);
    nextBtn_->setEnabled(currentPage_ < pageCount_);
}
