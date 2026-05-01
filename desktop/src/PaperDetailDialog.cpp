#include "PaperDetailDialog.hpp"
#include "FavoriteManager.hpp"
#include <QDesktopServices>
#include <QUrl>
#include <QClipboard>
#include <QApplication>

PaperDetailDialog::PaperDetailDialog(const Paper& paper, FavoriteManager* favMgr, QWidget* parent)
    : QDialog(parent)
    , paper_(paper)
    , favManager_(favMgr)
{
    setupUI(paper);
}

void PaperDetailDialog::setupUI(const Paper& paper) {
    setWindowTitle(paper.title.left(60) + (paper.title.length() > 60 ? "..." : ""));
    setMinimumSize(600, 550);
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    mainLayout->setSpacing(16);

    // Title
    auto* titleLabel = new QLabel(paper.title);
    titleLabel->setWordWrap(true);
    titleLabel->setStyleSheet(
        "font-size: 18px; font-weight: bold; color: palette(text); line-height: 1.4;"
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
    divider->setStyleSheet("color: palette(mid);");
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
        doiLabel->setStyleSheet("font-weight: bold; color: palette(mid); min-width: 80px;");
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

    // Favorite button
    if (favManager_) {
        favBtn_ = new QPushButton();
        updateFavoriteButton();
        connect(favBtn_, &QPushButton::clicked, this, [this]() {
            if (!favManager_) return;
            bool wasFav = favManager_->isFavorite(paper_.id);
            if (wasFav) {
                favManager_->removeFavorite(paper_.id);
            } else {
                QString journal = paper_.journalFull.isEmpty() ? paper_.journal : paper_.journalFull;
                favManager_->addFavorite(paper_.id, paper_.title, journal, paper_.year);
            }
            updateFavoriteButton();
            emit favoriteToggled(paper_.id, !wasFav);
        });
        buttonLayout->addWidget(favBtn_);
    }

    // Copy title button
    auto* copyBtn = new QPushButton("Copy Title");
    copyBtn->setStyleSheet(
        "QPushButton { background: palette(base); color: palette(text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 16px; font-weight: bold; }"
        "QPushButton:hover { background: palette(alternate-base); }"
    );
    connect(copyBtn, &QPushButton::clicked, this, [paper]() {
        QApplication::clipboard()->setText(paper.title);
    });
    buttonLayout->addWidget(copyBtn);

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
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 20px; font-weight: bold; }"
        "QPushButton:hover { background: palette(light); }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    buttonLayout->addWidget(closeBtn);

    mainLayout->addLayout(buttonLayout);
}

void PaperDetailDialog::updateFavoriteButton() {
    if (!favBtn_ || !favManager_) return;
    bool isFav = favManager_->isFavorite(paper_.id);
    favBtn_->setText(isFav ? "Remove from Favorites" : "Add to Favorites");
    favBtn_->setStyleSheet(
        isFav
        ? "QPushButton { background: #fbbf24; color: #78350f; border: none; "
          "border-radius: 6px; padding: 8px 16px; font-weight: bold; }"
          "QPushButton:hover { background: #f59e0b; }"
        : "QPushButton { background: palette(base); color: palette(text); border: 1px solid palette(mid); "
          "border-radius: 6px; padding: 8px 16px; font-weight: bold; }"
          "QPushButton:hover { background: #fef3c7; }"
    );
}

QWidget* PaperDetailDialog::createInfoRow(const QString& label, const QString& value) {
    auto* widget = new QWidget();
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* labelWidget = new QLabel(label);
    labelWidget->setStyleSheet("font-weight: bold; color: palette(mid); font-size: 12px;");

    auto* valueWidget = new QLabel(value);
    valueWidget->setWordWrap(true);
    valueWidget->setTextInteractionFlags(Qt::TextSelectableByMouse);
    valueWidget->setStyleSheet("color: palette(text); font-size: 14px; line-height: 1.5;");

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
