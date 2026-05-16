#include "tools/SplitPaperView.hpp"
#include "core/PaperTypes.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollBar>

SplitPaperView::SplitPaperView(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SplitPaperView::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // Toolbar
    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(8, 4, 8, 4);

    auto* titleLabel = new QLabel("Split View");
    titleLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    toolbar->addWidget(titleLabel);
    toolbar->addStretch();

    compareBtn_ = new QPushButton("Compare");
    compareBtn_->setEnabled(false);
    compareBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 4px; "
        "padding: 4px 12px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
        "QPushButton:disabled { background: palette(mid); }"
    );
    connect(compareBtn_, &QPushButton::clicked, this, [this]() {
        if (leftPaperId_ > 0 && rightPaperId_ > 0) {
            emit compareRequested(leftPaperId_, rightPaperId_);
        }
    });
    toolbar->addWidget(compareBtn_);
    layout->addLayout(toolbar);

    // Splitter with two panels
    splitter_ = new QSplitter(Qt::Horizontal);

    auto* leftPanel = createPaperPanel("Paper A");
    leftTitle_ = leftPanel->findChild<QLabel*>("panelTitle");
    leftContent_ = leftPanel->findChild<QTextEdit*>("panelContent");
    splitter_->addWidget(leftPanel);

    auto* rightPanel = createPaperPanel("Paper B");
    rightTitle_ = rightPanel->findChild<QLabel*>("panelTitle");
    rightContent_ = rightPanel->findChild<QTextEdit*>("panelContent");
    splitter_->addWidget(rightPanel);

    splitter_->setStretchFactor(0, 1);
    splitter_->setStretchFactor(1, 1);
    splitter_->setSizes({500, 500});

    layout->addWidget(splitter_, 1);
}

QWidget* SplitPaperView::createPaperPanel(const QString& label) {
    auto* panel = new QWidget();
    auto* layout = new QVBoxLayout(panel);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    auto* title = new QLabel(label);
    title->setObjectName("panelTitle");
    title->setStyleSheet("font-weight: bold; font-size: 13px; color: palette(text); padding: 4px;");
    layout->addWidget(title);

    auto* content = new QTextEdit();
    content->setObjectName("panelContent");
    content->setReadOnly(true);
    content->setStyleSheet(
        "QTextEdit { border: 1px solid palette(mid); border-radius: 6px; "
        "padding: 8px; font-size: 13px; background: palette(base); }"
    );
    layout->addWidget(content, 1);

    return panel;
}

void SplitPaperView::setLeftPaper(const Paper& paper) {
    leftPaperId_ = paper.id;
    leftTitle_->setText(QString("<b>%1</b> <span style='color: palette(mid);'>(%2)</span>")
        .arg(paper.title, paper.year));

    QString html;
    html += QString("<h3>%1</h3>").arg(paper.title);
    html += QString("<p><b>Authors:</b> %1</p>").arg(paper.authors);
    html += QString("<p><b>Journal:</b> %1</p>").arg(paper.journalFull.isEmpty() ? paper.journal : paper.journalFull);
    html += QString("<p><b>Year:</b> %1</p>").arg(paper.year);
    if (!paper.doiUrl.isEmpty())
        html += QString("<p><b>DOI:</b> %1</p>").arg(paper.doiUrl);
    html += QString("<p><b>Citations:</b> %1</p>").arg(paper.citationCount);
    html += QString("<hr><p>%1</p>").arg(paper.abstract);

    leftContent_->setHtml(html);
    compareBtn_->setEnabled(leftPaperId_ > 0 && rightPaperId_ > 0);
}

void SplitPaperView::setRightPaper(const Paper& paper) {
    rightPaperId_ = paper.id;
    rightTitle_->setText(QString("<b>%1</b> <span style='color: palette(mid);'>(%2)</span>")
        .arg(paper.title, paper.year));

    QString html;
    html += QString("<h3>%1</h3>").arg(paper.title);
    html += QString("<p><b>Authors:</b> %1</p>").arg(paper.authors);
    html += QString("<p><b>Journal:</b> %1</p>").arg(paper.journalFull.isEmpty() ? paper.journal : paper.journalFull);
    html += QString("<p><b>Year:</b> %1</p>").arg(paper.year);
    if (!paper.doiUrl.isEmpty())
        html += QString("<p><b>DOI:</b> %1</p>").arg(paper.doiUrl);
    html += QString("<p><b>Citations:</b> %1</p>").arg(paper.citationCount);
    html += QString("<hr><p>%1</p>").arg(paper.abstract);

    rightContent_->setHtml(html);
    compareBtn_->setEnabled(leftPaperId_ > 0 && rightPaperId_ > 0);
}

void SplitPaperView::clear() {
    leftPaperId_ = 0;
    rightPaperId_ = 0;
    leftTitle_->setText("Paper A");
    rightTitle_->setText("Paper B");
    leftContent_->clear();
    rightContent_->clear();
    compareBtn_->setEnabled(false);
}
