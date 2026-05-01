#include "PaperDetailDialog.hpp"
#include <QDesktopServices>
#include <QUrl>

PaperDetailDialog::PaperDetailDialog(const Paper& paper, QWidget* parent)
    : QDialog(parent)
    , paper_(paper)
{
    setupUI(paper);
}

void PaperDetailDialog::setupUI(const Paper& paper) {
    setWindowTitle(paper.title.left(60) + (paper.title.length() > 60 ? "..." : ""));
    setMinimumSize(600, 500);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // Title
    auto* titleLabel = new QLabel(paper.title);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: #1e293b; line-height: 1.4;"
    );
    mainLayout->addWidget(titleLabel);

    // Badges row
    auto* badgeLayout = new QHBoxLayout();
    badgeLayout->setSpacing(8);

    if (!paper.level.isEmpty()) {
        QString color = paper.level.toUpper() == "A" ? "#dc2626" :
                        paper.level.toUpper() == "B" ? "#ea580c" :
                        paper.level.toUpper() == "C" ? "#ca8a04" : "#6b7280";
        badgeLayout->addWidget(createBadge("CCF-" + paper.level.toUpper(), color));
    }
    if (!paper.year.isEmpty()) {
        badgeLayout->addWidget(createBadge(paper.year, "#4f46e5"));
    }
    if (!paper.type.isEmpty()) {
        badgeLayout->addWidget(createBadge(paper.type, "#0891b2"));
    }
    badgeLayout->addStretch();
    mainLayout->addLayout(badgeLayout);

    // Divider
    auto* divider = new QFrame();
    divider->setFrameShape(QFrame::HLine);
    divider->setStyleSheet("color: #e2e8f0;");
    mainLayout->addWidget(divider);

    // Scroll area for details
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget();
    auto* detailLayout = new QVBoxLayout(scrollContent);
    detailLayout->setSpacing(12);

    // Authors
    if (!paper.authors.isEmpty()) {
        detailLayout->addWidget(createInfoRow("Authors", paper.authors));
    }

    // Journal
    QString journal = paper.journalFull.isEmpty() ? paper.journal : paper.journalFull;
    if (!journal.isEmpty()) {
        detailLayout->addWidget(createInfoRow("Journal", journal));
    }

    // DOI
    if (!paper.doiUrl.isEmpty()) {
        auto* doiRow = new QHBoxLayout();
        auto* doiLabel = new QLabel("DOI:");
        doiLabel->setStyleSheet("font-weight: bold; color: #64748b; min-width: 80px;");
        auto* doiLink = new QLabel(QString("<a href=\"%1\">%1</a>").arg(paper.doiUrl));
        doiLink->setOpenExternalLinks(true);
        doiLink->setWordWrap(true);
        doiRow->addWidget(doiLabel);
        doiRow->addWidget(doiLink, 1);
        detailLayout->addLayout(doiRow);
    }

    // Abstract
    if (!paper.abstract.isEmpty()) {
        detailLayout->addWidget(createInfoRow("Abstract", paper.abstract));
    }

    detailLayout->addStretch();
    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    if (!paper.doiUrl.isEmpty()) {
        auto* openLinkBtn = new QPushButton("Open DOI");
        openLinkBtn->setStyleSheet(
            "QPushButton { background: #4f46e5; color: white; border: none; "
            "border-radius: 6px; padding: 8px 20px; font-weight: bold; }"
            "QPushButton:hover { background: #4338ca; }"
        );
        connect(openLinkBtn, &QPushButton::clicked, this, [paper]() {
            QDesktopServices::openUrl(QUrl(paper.doiUrl));
        });
        buttonLayout->addWidget(openLinkBtn);
    }

    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: #f1f5f9; color: #475569; border: 1px solid #e2e8f0; "
        "border-radius: 6px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: #e2e8f0; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addWidget(closeBtn);

    mainLayout->addLayout(buttonLayout);
}

QWidget* PaperDetailDialog::createInfoRow(const QString& label, const QString& value) {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-weight: bold; color: #64748b; font-size: 12px;");

    auto* valueWidget = new QLabel(value);
    valueWidget->setWordWrap(true);
    valueWidget->setStyleSheet("color: #1e293b; font-size: 14px; line-height: 1.5;");

    layout->addWidget(labelWidget);
    layout->addWidget(valueWidget);

    return widget;
}

QWidget* PaperDetailDialog::createBadge(const QString& text, const QString& color) {
    auto* badge = new QLabel(text);
    badge->setAlignment(Qt::AlignCenter);
    badge->setStyleSheet(
        QString("background: %1; color: white; border-radius: 10px; "
                "padding: 3px 12px; font-size: 12px; font-weight: bold;")
        .arg(color)
    );
    return badge;
}
