#include "paper/FavoritesDialog.hpp"
#include "paper/FavoriteManager.hpp"
#include <QScrollArea>

FavoritesDialog::FavoritesDialog(FavoriteManager* favoriteManager, QWidget* parent)
    : QDialog(parent)
    , favoriteManager_(favoriteManager)
{
    setWindowTitle("My Favorites");
    setMinimumSize(600, 500);
    setModal(true);
    setupUI();
    loadFavorites();
}

void FavoritesDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // Header
    auto* headerLayout = new QHBoxLayout();
    auto* titleLabel = new QLabel("My Favorites");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold; color: palette(text);");
    headerLayout->addWidget(titleLabel);

    countLabel_ = new QLabel("0 papers");
    countLabel_->setStyleSheet("color: palette(mid); font-size: 14px;");
    headerLayout->addWidget(countLabel_);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    // Search/filter
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search favorites...");
    searchEdit_->setStyleSheet(
        "QLineEdit { padding: 8px 16px; border: 1px solid palette(mid); "
        "border-radius: 8px; font-size: 13px; }"
    );
    connect(searchEdit_, &QLineEdit::textChanged, this, [this](const QString& text) {
        Q_UNUSED(text);
        loadFavorites();
    });
    mainLayout->addWidget(searchEdit_);

    // Scrollable list
    auto* scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    auto* scrollContent = new QWidget();
    listLayout_ = new QVBoxLayout(scrollContent);
    listLayout_->setSpacing(8);
    listLayout_->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(scrollContent);
    mainLayout->addWidget(scrollArea, 1);

    // Close button
    auto* closeBtn = new QPushButton("Close");
    closeBtn->setStyleSheet(
        "QPushButton { background: palette(button); color: palette(button-text); border: 1px solid palette(mid); "
        "border-radius: 6px; padding: 8px 24px; font-weight: bold; }"
        "QPushButton:hover { background: palette(light); }"
    );
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);

    // Refresh on favorite changes
    if (favoriteManager_) {
        connect(favoriteManager_, &FavoriteManager::favoriteAdded, this, [this]() { loadFavorites(); });
        connect(favoriteManager_, &FavoriteManager::favoriteRemoved, this, [this]() { loadFavorites(); });
    }
}

void FavoritesDialog::loadFavorites() {
    // Clear existing items
    QLayoutItem* item;
    while ((item = listLayout_->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }

    if (!favoriteManager_) return;

    auto favorites = favoriteManager_->getFavorites();
    QString filter = searchEdit_ ? searchEdit_->text().trimmed().toLower() : "";

    int shown = 0;
    for (const auto& fav : favorites) {
        if (!filter.isEmpty() && !fav.title.toLower().contains(filter)
            && !fav.journal.toLower().contains(filter)) {
            continue;
        }
        listLayout_->addWidget(createFavoriteItem(fav.title, fav.journal, fav.year, fav.paperId));
        shown++;
    }

    countLabel_->setText(QString("%1 paper(s)").arg(shown));
}

QWidget* FavoritesDialog::createFavoriteItem(const QString& title, const QString& journal,
                                              const QString& year, int paperId) {
    auto* card = new QWidget();
    card->setObjectName("favCard");
    card->setStyleSheet(
        "QWidget#favCard { background: palette(base); border: 1px solid palette(mid); "
        "border-radius: 8px; padding: 12px 16px; }"
        "QWidget#favCard:hover { border-color: #818cf8; }"
    );

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* titleLabel = new QLabel(title);
    titleLabel->setStyleSheet("font-weight: bold; color: palette(text); font-size: 14px;");
    titleLabel->setWordWrap(true);
    layout->addWidget(titleLabel);

    auto* metaLayout = new QHBoxLayout();
    auto* journalLabel = new QLabel(journal);
    journalLabel->setStyleSheet("color: palette(mid); font-size: 12px;");

    auto* yearLabel = new QLabel(year);
    yearLabel->setStyleSheet("color: palette(mid); font-size: 12px;");

    metaLayout->addWidget(journalLabel);
    metaLayout->addWidget(yearLabel);
    metaLayout->addStretch();

    // Remove button
    auto* removeBtn = new QPushButton("Remove");
    removeBtn->setStyleSheet(
        "QPushButton { background: none; border: none; color: #ef4444; "
        "font-size: 11px; font-weight: bold; }"
        "QPushButton:hover { color: #dc2626; text-decoration: underline; }"
    );
    connect(removeBtn, &QPushButton::clicked, this, [this, paperId]() {
        if (favoriteManager_) {
            favoriteManager_->removeFavorite(paperId);
        }
    });
    metaLayout->addWidget(removeBtn);

    layout->addLayout(metaLayout);

    return card;
}
