#include "search/AdvancedSearchDialog.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QRegularExpression>

AdvancedSearchDialog::AdvancedSearchDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Advanced Search");
    resize(600, 550);
    setupUI();
}

void AdvancedSearchDialog::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);

    // Search query
    auto* queryGroup = new QGroupBox("Search");
    auto* queryLayout = new QVBoxLayout(queryGroup);

    queryEdit_ = new QLineEdit();
    queryEdit_->setPlaceholderText("Full text search query...");
    queryEdit_->setStyleSheet("padding: 8px; font-size: 14px;");
    queryLayout->addWidget(queryEdit_);
    mainLayout->addWidget(queryGroup);

    // Field-specific filters
    auto* filterGroup = new QGroupBox("Filters");
    auto* filterLayout = new QFormLayout(filterGroup);

    titleEdit_ = new QLineEdit();
    titleEdit_->setPlaceholderText("Filter by title...");
    filterLayout->addRow("Title:", titleEdit_);

    authorEdit_ = new QLineEdit();
    authorEdit_->setPlaceholderText("Filter by author...");
    filterLayout->addRow("Author:", authorEdit_);

    abstractEdit_ = new QLineEdit();
    abstractEdit_->setPlaceholderText("Filter by abstract...");
    filterLayout->addRow("Abstract:", abstractEdit_);

    venueEdit_ = new QLineEdit();
    venueEdit_->setPlaceholderText("Filter by venue/journal...");
    filterLayout->addRow("Venue:", venueEdit_);

    auto* yearRow = new QHBoxLayout();
    yearFromEdit_ = new QLineEdit();
    yearFromEdit_->setPlaceholderText("From");
    yearFromEdit_->setMaximumWidth(80);
    yearRow->addWidget(yearFromEdit_);
    yearRow->addWidget(new QLabel("—"));
    yearToEdit_ = new QLineEdit();
    yearToEdit_->setPlaceholderText("To");
    yearToEdit_->setMaximumWidth(80);
    yearRow->addWidget(yearToEdit_);
    yearRow->addStretch();
    filterLayout->addRow("Year:", yearRow);

    levelCombo_ = new QComboBox();
    levelCombo_->addItem("Any Level", "");
    levelCombo_->addItem("CCFS-A", "A");
    levelCombo_->addItem("CCFS-B", "B");
    levelCombo_->addItem("CCFS-C", "C");
    levelCombo_->addItem("Non-CCFS", "none");
    filterLayout->addRow("Level:", levelCombo_);

    mainLayout->addWidget(filterGroup);

    // Options
    auto* optionsGroup = new QGroupBox("Options");
    auto* optionsLayout = new QVBoxLayout(optionsGroup);

    auto* sortRow = new QHBoxLayout();
    sortFieldCombo_ = new QComboBox();
    sortFieldCombo_->addItems({"Relevance", "Date", "Title", "Citations"});
    sortRow->addWidget(new QLabel("Sort:"));
    sortRow->addWidget(sortFieldCombo_);

    sortOrderCombo_ = new QComboBox();
    sortOrderCombo_->addItems({"Descending", "Ascending"});
    sortRow->addWidget(sortOrderCombo_);

    sortRow->addStretch();
    limitSpin_ = new QSpinBox();
    limitSpin_->setRange(5, 100);
    limitSpin_->setValue(20);
    sortRow->addWidget(new QLabel("Limit:"));
    sortRow->addWidget(limitSpin_);
    optionsLayout->addLayout(sortRow);

    fullTextCheck_ = new QCheckBox("Full text available only");
    optionsLayout->addWidget(fullTextCheck_);

    openAccessCheck_ = new QCheckBox("Open access only");
    optionsLayout->addWidget(openAccessCheck_);

    mainLayout->addWidget(optionsGroup);

    // Recent searches
    auto* recentGroup = new QGroupBox("Recent Searches");
    auto* recentLayout = new QVBoxLayout(recentGroup);
    recentSearchesList_ = new QListWidget();
    recentSearchesList_->setMaximumHeight(100);
    connect(recentSearchesList_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        queryEdit_->setText(item->text());
    });
    recentLayout->addWidget(recentSearchesList_);
    mainLayout->addWidget(recentGroup);

    // Buttons
    auto* btnRow = new QHBoxLayout();

    auto* saveBtn = new QPushButton("Save Search");
    connect(saveBtn, &QPushButton::clicked, this, &AdvancedSearchDialog::onSaveSearch);
    btnRow->addWidget(saveBtn);

    auto* loadBtn = new QPushButton("Load Saved");
    connect(loadBtn, &QPushButton::clicked, this, &AdvancedSearchDialog::onLoadSaved);
    btnRow->addWidget(loadBtn);

    btnRow->addStretch();

    auto* buttonBox = new QDialogButtonBox(QDialogButtonBox::Search | QDialogButtonBox::Close);
    connect(buttonBox->button(QDialogButtonBox::Search), &QPushButton::clicked, this, &AdvancedSearchDialog::onSearch);
    connect(buttonBox->button(QDialogButtonBox::Close), &QPushButton::clicked, this, &QDialog::reject);
    btnRow->addWidget(buttonBox);

    mainLayout->addLayout(btnRow);
}

AdvancedSearchDialog::SearchCriteria AdvancedSearchDialog::criteria() const {
    SearchCriteria c;
    c.query = queryEdit_->text().trimmed();
    c.title = titleEdit_->text().trimmed();
    c.author = authorEdit_->text().trimmed();
    c.abstract = abstractEdit_->text().trimmed();
    c.venue = venueEdit_->text().trimmed();
    c.yearFrom = yearFromEdit_->text().trimmed();
    c.yearTo = yearToEdit_->text().trimmed();
    c.level = levelCombo_->currentData().toString();
    c.sortField = sortFieldCombo_->currentText().toLower();
    c.sortOrder = sortOrderCombo_->currentIndex() == 0 ? "desc" : "asc";
    c.limit = limitSpin_->value();
    c.fullTextOnly = fullTextCheck_->isChecked();
    c.openAccessOnly = openAccessCheck_->isChecked();
    return c;
}

void AdvancedSearchDialog::onSearch() {
    if (criteria().query.isEmpty() && criteria().title.isEmpty() &&
        criteria().author.isEmpty() && criteria().abstract.isEmpty()) {
        return;
    }

    // Add to recent
    QString queryText = criteria().query;
    if (queryText.isEmpty()) queryText = QString("title:%1 author:%2").arg(criteria().title, criteria().author);
    auto* item = new QListWidgetItem(queryText);
    recentSearchesList_->insertItem(0, item);
    if (recentSearchesList_->count() > 20) {
        delete recentSearchesList_->takeItem(recentSearchesList_->count() - 1);
    }

    emit searchRequested(criteria());
    accept();
}

void AdvancedSearchDialog::onSaveSearch() {
    // Delegate to API
    accept();
}

void AdvancedSearchDialog::onLoadSaved() {
    // Delegate to API
}

void AdvancedSearchDialog::onAddFilterRow() {}

void AdvancedSearchDialog::onRemoveFilterRow() {}
