#include "search/SmartSearchWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QRegularExpression>

SmartSearchWidget::SmartSearchWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SmartSearchWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Search bar
    auto* searchRow = new QHBoxLayout();

    fieldCombo_ = new QComboBox();
    fieldCombo_->addItems({"All", "Title", "Author", "Year", "Journal", "DOI", "Abstract"});
    fieldCombo_->setFixedWidth(100);
    searchRow->addWidget(fieldCombo_);

    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Smart search... (author:\"Smith\" year:>2020 -review)");
    searchEdit_->setStyleSheet(
        "QLineEdit { padding: 8px; border: 2px solid #3b82f6; border-radius: 6px; font-size: 13px; }"
    );
    connect(searchEdit_, &QLineEdit::textChanged, this, &SmartSearchWidget::onTextChanged);
    connect(searchEdit_, &QLineEdit::returnPressed, this, &SmartSearchWidget::onSearch);
    searchRow->addWidget(searchEdit_, 1);

    auto* searchBtn = new QPushButton("Search");
    searchBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(searchBtn, &QPushButton::clicked, this, &SmartSearchWidget::onSearch);
    searchRow->addWidget(searchBtn);

    auto* clearBtn = new QPushButton("Clear");
    connect(clearBtn, &QPushButton::clicked, this, &SmartSearchWidget::onClear);
    searchRow->addWidget(clearBtn);

    layout->addLayout(searchRow);

    // Parsed tokens
    parsedLabel_ = new QLabel("");
    parsedLabel_->setStyleSheet("font-size: 11px; color: #64748b; padding: 4px;");
    parsedLabel_->setWordWrap(true);
    layout->addWidget(parsedLabel_);

    // Token list + suggestions side by side
    auto* midRow = new QHBoxLayout();

    // Parsed tokens
    auto* tokenPanel = new QVBoxLayout();
    tokenPanel->addWidget(new QLabel("Parsed Tokens:"));
    tokenList_ = new QListWidget();
    tokenList_->setMaximumHeight(120);
    tokenList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; font-size: 11px; }"
    );
    tokenPanel->addWidget(tokenList_);
    midRow->addLayout(tokenPanel, 1);

    // Suggestions
    auto* sugPanel = new QVBoxLayout();
    sugPanel->addWidget(new QLabel("Suggestions:"));
    suggestList_ = new QListWidget();
    suggestList_->setMaximumHeight(120);
    suggestList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; font-size: 11px; }"
        "QListWidget::item:hover { background: #dbeafe; }"
    );
    connect(suggestList_, &QListWidget::itemClicked, this, &SmartSearchWidget::onSuggestionClicked);
    sugPanel->addWidget(suggestList_);
    midRow->addLayout(sugPanel, 1);

    layout->addLayout(midRow);

    // Stats + save/load
    auto* bottomRow = new QHBoxLayout();
    statsLabel_ = new QLabel("Ready");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    bottomRow->addWidget(statsLabel_, 1);

    auto* saveBtn = new QPushButton("Save Search");
    connect(saveBtn, &QPushButton::clicked, this, &SmartSearchWidget::onSaveSearch);
    bottomRow->addWidget(saveBtn);

    auto* loadBtn = new QPushButton("Load Search");
    connect(loadBtn, &QPushButton::clicked, this, &SmartSearchWidget::onLoadSearch);
    bottomRow->addWidget(loadBtn);

    layout->addLayout(bottomRow);
}

void SmartSearchWidget::setSearchHistory(const QStringList& history) {
    history_ = history.mid(0, MAX_HISTORY);
}

QStringList SmartSearchWidget::searchHistory() const {
    return history_;
}

void SmartSearchWidget::setSavedSearches(const QList<QPair<QString, QString>>& searches) {
    savedSearches_ = searches;
}

void SmartSearchWidget::addSavedSearch(const QString& name, const QString& query) {
    savedSearches_.append({name, query});
    emit searchSaved(name, query);
}

QList<SearchToken> SmartSearchWidget::parseQuery(const QString& query) const {
    QList<SearchToken> tokens;
    QRegularExpression re(R"((-\s*)?(?:(\w+):)?"([^"]*)"|(\S+))");
    auto it = re.globalMatch(query);
    while (it.hasNext()) {
        auto match = it.next();
        SearchToken token;
        token.negated = !match.captured(1).isEmpty();
        QString field = match.captured(2).toLower();
        QString value = match.captured(3).isEmpty() ? match.captured(4) : match.captured(3);

        if (field == "author" || field == "au") token.type = "author";
        else if (field == "year" || field == "yr") token.type = "year";
        else if (field == "journal" || field == "j") token.type = "journal";
        else if (field == "doi") token.type = "doi";
        else if (field == "title" || field == "t") token.type = "title";
        else if (field == "abstract" || field == "abs") token.type = "abstract";
        else token.type = "keyword";

        token.value = value;
        if (!token.value.isEmpty()) tokens.append(token);
    }
    return tokens;
}

QString SmartSearchWidget::buildQuery(const QList<SearchToken>& tokens) const {
    QStringList parts;
    for (const auto& t : tokens) {
        QString prefix = t.negated ? "-" : "";
        if (t.type == "keyword") parts << prefix + t.value;
        else parts << prefix + t.type + ":\"" + t.value + "\"";
    }
    return parts.join(" ");
}

QStringList SmartSearchWidget::suggestions(const QString& partial) const {
    QStringList results;

    // Field hints
    if (partial.endsWith(":")) {
        QString field = partial.left(partial.size() - 1).toLower();
        static QMap<QString, QStringList> fieldHints = {
            {"author", {"author:\"Smith\"", "author:\"Zhang\""}},
            {"year", {"year:>2020", "year:2023", "year:2022"}},
            {"journal", {"journal:\"Nature\"", "journal:\"Science\""}},
            {"doi", {"doi:10.1038/"}},
            {"title", {"title:\"deep learning\""}},
        };
        results = fieldHints.value(field);
    }

    // History matches
    for (const auto& h : history_) {
        if (h.toLower().contains(partial.toLower()) && results.size() < 10) {
            results << "H: " + h;
        }
    }

    return results;
}

void SmartSearchWidget::onSearch() {
    QString query = searchEdit_->text().trimmed();
    if (query.isEmpty()) return;

    addToHistory(query);
    auto tokens = parseQuery(query);
    emit searchRequested(query);
    emit smartSearchRequested(tokens);
    statsLabel_->setText(QString("Searched: %1 (%2 tokens)").arg(query).arg(tokens.size()));
}

void SmartSearchWidget::onClear() {
    searchEdit_->clear();
    tokenList_->clear();
    suggestList_->clear();
    parsedLabel_->clear();
}

void SmartSearchWidget::onSaveSearch() {
    QString name = QInputDialog::getText(this, "Save Search", "Name:");
    if (name.trimmed().isEmpty()) return;
    addSavedSearch(name.trimmed(), searchEdit_->text());
}

void SmartSearchWidget::onLoadSearch() {
    if (savedSearches_.isEmpty()) return;
    QStringList names;
    for (const auto& s : savedSearches_) names << s.first;
    QString name = QInputDialog::getItem(this, "Load Search", "Select:", names, 0, false);
    for (const auto& s : savedSearches_) {
        if (s.first == name) {
            searchEdit_->setText(s.second);
            break;
        }
    }
}

void SmartSearchWidget::onSuggestionClicked(QListWidgetItem* item) {
    if (!item) return;
    QString text = item->text();
    if (text.startsWith("H: ")) text = text.mid(3);
    searchEdit_->setText(text);
}

void SmartSearchWidget::onTextChanged(const QString& text) {
    updateSuggestions(text);

    auto tokens = parseQuery(text);
    tokenList_->clear();
    parsedLabel_->clear();

    QStringList displayTokens;
    for (const auto& t : tokens) {
        QString color = t.negated ? "#ef4444" : (t.type == "keyword" ? "#64748b" : "#3b82f6");
        QString display = QString("%1%2:%3")
            .arg(t.negated ? "NOT " : "")
            .arg(t.type)
            .arg(t.value.left(30));
        auto* item = new QListWidgetItem(display);
        item->setForeground(QColor(color));
        tokenList_->addItem(item);
        displayTokens << display;
    }

    if (!tokens.isEmpty()) {
        parsedLabel_->setText("Parsed: " + displayTokens.join(" | "));
    }
}

void SmartSearchWidget::updateSuggestions(const QString& text) {
    suggestList_->clear();
    auto sugs = suggestions(text);
    for (const auto& s : sugs) {
        suggestList_->addItem(s);
    }
}

void SmartSearchWidget::addToHistory(const QString& query) {
    history_.removeAll(query);
    history_.prepend(query);
    while (history_.size() > MAX_HISTORY) history_.removeLast();
}
