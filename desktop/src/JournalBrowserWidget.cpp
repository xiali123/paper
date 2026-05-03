#include "JournalBrowserWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <algorithm>

JournalBrowserWidget::JournalBrowserWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void JournalBrowserWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Search + sort row
    auto* topRow = new QHBoxLayout();

    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search journals...");
    connect(searchEdit_, &QLineEdit::textChanged, this, &JournalBrowserWidget::onSearchChanged);
    topRow->addWidget(searchEdit_, 1);

    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Sort: Name", "Sort: Impact Factor", "Sort: Paper Count", "Sort: Field"});
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &JournalBrowserWidget::onSortChanged);
    topRow->addWidget(sortCombo_);

    favBtn_ = new QPushButton("Favorite");
    favBtn_->setCheckable(true);
    connect(favBtn_, &QPushButton::toggled, this, &JournalBrowserWidget::onToggleFavorite);
    topRow->addWidget(favBtn_);

    searchBtn_ = new QPushButton("Search Papers");
    searchBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; "
        "border-radius: 4px; }"
    );
    connect(searchBtn_, &QPushButton::clicked, this, &JournalBrowserWidget::onSearchPapers);
    topRow->addWidget(searchBtn_);

    layout->addLayout(topRow);

    // Tree
    journalTree_ = new QTreeWidget();
    journalTree_->setHeaderLabels({"Journal", "Field", "IF", "Papers", "Publisher"});
    journalTree_->header()->setStretchLastSection(true);
    journalTree_->setColumnWidth(0, 200);
    journalTree_->setColumnWidth(1, 100);
    journalTree_->setColumnWidth(2, 50);
    journalTree_->setColumnWidth(3, 50);
    journalTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(journalTree_, &QTreeWidget::itemClicked, this, &JournalBrowserWidget::onJournalClicked);
    layout->addWidget(journalTree_, 1);

    statsLabel_ = new QLabel("0 journals");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void JournalBrowserWidget::setJournals(const QList<JournalInfo>& journals) {
    journals_ = journals;
    refreshTree();
    updateStats();
}

QList<JournalInfo> JournalBrowserWidget::journals() const {
    return journals_;
}

void JournalBrowserWidget::setFavorites(const QStringList& issns) {
    favoriteIssns_ = QSet<QString>(issns.begin(), issns.end());
    refreshTree();
}

QStringList JournalBrowserWidget::favorites() const {
    return favoriteIssns_.values();
}

void JournalBrowserWidget::onSearchChanged(const QString& text) {
    Q_UNUSED(text);
    refreshTree();
}

void JournalBrowserWidget::onSortChanged(int index) {
    switch (index) {
        case 0:
            std::sort(journals_.begin(), journals_.end(),
                [](const JournalInfo& a, const JournalInfo& b) { return a.name < b.name; });
            break;
        case 1:
            std::sort(journals_.begin(), journals_.end(),
                [](const JournalInfo& a, const JournalInfo& b) { return a.impactFactor > b.impactFactor; });
            break;
        case 2:
            std::sort(journals_.begin(), journals_.end(),
                [](const JournalInfo& a, const JournalInfo& b) { return a.paperCount > b.paperCount; });
            break;
        case 3:
            std::sort(journals_.begin(), journals_.end(),
                [](const JournalInfo& a, const JournalInfo& b) { return a.field < b.field; });
            break;
    }
    refreshTree();
}

void JournalBrowserWidget::onJournalClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
    selectedIssn_ = item->data(0, Qt::UserRole).toString();
    if (selectedIssn_.isEmpty()) {
        // Category header clicked
        favBtn_->setEnabled(false);
        searchBtn_->setEnabled(false);
        return;
    }
    favBtn_->setEnabled(true);
    searchBtn_->setEnabled(true);
    favBtn_->setChecked(favoriteIssns_.contains(selectedIssn_));
    emit journalSelected(selectedIssn_);
}

void JournalBrowserWidget::onToggleFavorite() {
    if (selectedIssn_.isEmpty()) return;
    if (favBtn_->isChecked()) {
        favoriteIssns_.insert(selectedIssn_);
    } else {
        favoriteIssns_.remove(selectedIssn_);
    }
    emit favoriteToggled(selectedIssn_, favBtn_->isChecked());
    refreshTree();
}

void JournalBrowserWidget::onSearchPapers() {
    for (const auto& j : journals_) {
        if (j.issn == selectedIssn_) {
            emit searchPapersInJournal(j.name);
            return;
        }
    }
}

void JournalBrowserWidget::refreshTree() {
    journalTree_->clear();
    QString filter = searchEdit_->text().trimmed().toLower();

    // Group by field
    QMap<QString, QList<JournalInfo>> grouped;
    for (const auto& j : journals_) {
        if (!filter.isEmpty() &&
            !j.name.toLower().contains(filter) &&
            !j.abbreviation.toLower().contains(filter) &&
            !j.field.toLower().contains(filter)) continue;
        grouped[j.field.isEmpty() ? "Other" : j.field].append(j);
    }

    for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it) {
        auto* catItem = new QTreeWidgetItem({it.key(), "", "", "", ""});
        QFont font;
        font.setBold(true);
        catItem->setFont(0, font);
        catItem->setExpanded(!filter.isEmpty());
        journalTree_->addTopLevelItem(catItem);

        for (const auto& j : it.value()) {
            QString name = j.name;
            if (favoriteIssns_.contains(j.issn)) name = "★ " + name;
            auto* item = new QTreeWidgetItem({
                name, j.field,
                QString::number(j.impactFactor, 'f', 1),
                QString::number(j.paperCount),
                j.publisher
            });
            item->setData(0, Qt::UserRole, j.issn);
            catItem->addChild(item);
        }
    }
}

void JournalBrowserWidget::updateStats() {
    statsLabel_->setText(QString("%1 journals in %2 fields")
        .arg(journals_.size()).arg(
            QSet<QString>::fromList([this]() {
                QStringList fields;
                for (const auto& j : journals_) fields << j.field;
                return fields;
            }()).size()));
}
