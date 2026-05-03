#include "SearchQueryBuilder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

SearchQueryBuilder::SearchQueryBuilder(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void SearchQueryBuilder::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Input row
    auto* inputRow = new QHBoxLayout();

    inputRow->addWidget(new QLabel("Field:"));
    fieldCombo_ = new QComboBox();
    fieldCombo_->setMinimumWidth(120);
    setFields({"title", "abstract", "authors", "journal", "year", "doi", "keywords", "fulltext"});
    inputRow->addWidget(fieldCombo_);

    inputRow->addWidget(new QLabel("Op:"));
    opCombo_ = new QComboBox();
    opCombo_->addItems({"contains", "equals", "starts with", "ends with", "not contains",
                        ">", ">=", "<", "<=", "between", "in", "regex"});
    opCombo_->setMinimumWidth(100);
    inputRow->addWidget(opCombo_);

    inputRow->addWidget(new QLabel("Value:"));
    valueEdit_ = new QLineEdit();
    valueEdit_->setPlaceholderText("Enter search value...");
    valueEdit_->setMinimumWidth(150);
    connect(valueEdit_, &QLineEdit::returnPressed, this, &SearchQueryBuilder::onAdd);
    inputRow->addWidget(valueEdit_, 1);

    inputRow->addWidget(new QLabel("Logic:"));
    logicCombo_ = new QComboBox();
    logicCombo_->addItems({"AND", "OR", "NOT"});
    logicCombo_->setMaximumWidth(70);
    inputRow->addWidget(logicCombo_);

    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &SearchQueryBuilder::onAdd);
    inputRow->addWidget(addBtn_);

    layout->addLayout(inputRow);

    // Clause list + preview
    auto* splitter = new QSplitter(Qt::Vertical);

    auto* topPanel = new QVBoxLayout();
    auto* listHeader = new QHBoxLayout();
    listHeader->addWidget(new QLabel("Query Clauses:"), 1);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &SearchQueryBuilder::onRemove);
    listHeader->addWidget(removeBtn_);

    clearBtn_ = new QPushButton("Clear All");
    connect(clearBtn_, &QPushButton::clicked, this, &SearchQueryBuilder::onClear);
    listHeader->addWidget(clearBtn_);

    topPanel->addLayout(listHeader);

    clauseList_ = new QListWidget();
    clauseList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; font-family: monospace; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    topPanel->addWidget(clauseList_);

    countLabel_ = new QLabel("0 clauses");
    countLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    topPanel->addWidget(countLabel_);

    auto* topWidget = new QWidget();
    topWidget->setLayout(topPanel);
    splitter->addWidget(topWidget);

    auto* bottomPanel = new QVBoxLayout();
    bottomPanel->addWidget(new QLabel("Generated Query:"));
    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Consolas", 10));
    previewEdit_->setMaximumHeight(150);
    previewEdit_->setStyleSheet("QTextEdit { background: #1e293b; color: #e2e8f0; padding: 8px; }");
    bottomPanel->addWidget(previewEdit_);

    auto* bottomWidget = new QWidget();
    bottomWidget->setLayout(bottomPanel);
    splitter->addWidget(bottomWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    searchBtn_ = new QPushButton("Search");
    searchBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 8px 24px; "
        "border-radius: 6px; font-weight: bold; font-size: 14px; }"
    );
    connect(searchBtn_, &QPushButton::clicked, this, &SearchQueryBuilder::onSearch);
    layout->addWidget(searchBtn_);
}

void SearchQueryBuilder::addClause(const QString& field, const QString& op, const QString& value, const QString& logic) {
    if (value.trimmed().isEmpty()) return;
    QueryClause clause;
    clause.id = nextId_++;
    clause.field = field;
    clause.operator_ = op;
    clause.value = value;
    clause.logic = logic;
    clauses_.append(clause);
    refreshList();
    updatePreview();
    emit queryBuilt(buildQuery());
}

void SearchQueryBuilder::clearClauses() {
    clauses_.clear();
    refreshList();
    updatePreview();
}

QList<QueryClause> SearchQueryBuilder::clauses() const { return clauses_; }

QString SearchQueryBuilder::buildQuery() const {
    if (clauses_.isEmpty()) return "";

    QStringList parts;
    for (int i = 0; i < clauses_.size(); ++i) {
        const auto& c = clauses_[i];
        QString clause;
        if (c.operator_ == "contains") clause = QString("%1:*%2*").arg(c.field, c.value);
        else if (c.operator_ == "equals") clause = QString("%1:\"%2\"").arg(c.field, c.value);
        else if (c.operator_ == "starts with") clause = QString("%1:%2*").arg(c.field, c.value);
        else if (c.operator_ == "ends with") clause = QString("%1:*%2").arg(c.field, c.value);
        else if (c.operator_ == "not contains") clause = QString("NOT %1:*%2*").arg(c.field, c.value);
        else if (c.operator_ == ">") clause = QString("%1:>%2").arg(c.field, c.value);
        else if (c.operator_ == ">=") clause = QString("%1:>=%2").arg(c.field, c.value);
        else if (c.operator_ == "<") clause = QString("%1:<%2").arg(c.field, c.value);
        else if (c.operator_ == "<=") clause = QString("%1:<=%2").arg(c.field, c.value);
        else if (c.operator_ == "between") clause = QString("%1:[%2]").arg(c.field, c.value);
        else if (c.operator_ == "in") clause = QString("%1:(%2)").arg(c.field, c.value);
        else if (c.operator_ == "regex") clause = QString("%1:/%2/").arg(c.field, c.value);
        else clause = QString("%1:%2").arg(c.field, c.value);

        if (i > 0 && c.logic != "NOT") parts << c.logic;
        parts << clause;
    }
    return parts.join(" ");
}

QString SearchQueryBuilder::buildElasticsearchQuery() const {
    if (clauses_.isEmpty()) return "{}";

    QJsonArray mustArr, shouldArr, mustNotArr;
    for (const auto& c : clauses_) {
        QJsonObject matchObj;
        QJsonObject fieldObj;

        if (c.operator_ == "contains" || c.operator_ == "equals") {
            fieldObj[c.field] = c.value;
            matchObj["match"] = fieldObj;
        } else if (c.operator_ == "starts with") {
            fieldObj[c.field] = c.value + "*";
            matchObj["wildcard"] = fieldObj;
        } else if (c.operator_ == ">=" || c.operator_ == "<=" || c.operator_ == ">" || c.operator_ == "<") {
            QJsonObject rangeContent;
            rangeContent[c.operator_] = c.value;
            QJsonObject rangeField;
            rangeField[c.field] = rangeContent;
            matchObj["range"] = rangeField;
        } else {
            fieldObj[c.field] = c.value;
            matchObj["match"] = fieldObj;
        }

        if (c.logic == "AND") mustArr.append(matchObj);
        else if (c.logic == "OR") shouldArr.append(matchObj);
        else if (c.logic == "NOT") mustNotArr.append(matchObj);
    }

    QJsonObject boolObj;
    if (!mustArr.isEmpty()) boolObj["must"] = mustArr;
    if (!shouldArr.isEmpty()) boolObj["should"] = shouldArr;
    if (!mustNotArr.isEmpty()) boolObj["must_not"] = mustNotArr;

    QJsonObject queryObj;
    queryObj["bool"] = boolObj;
    QJsonObject root;
    root["query"] = queryObj;

    return QJsonDocument(root).toJson(QJsonDocument::Indented);
}

void SearchQueryBuilder::setFields(const QStringList& fields) {
    fieldCombo_->clear();
    for (const auto& f : fields) fieldCombo_->addItem(f);
}

void SearchQueryBuilder::onAdd() {
    addClause(fieldCombo_->currentText(), opCombo_->currentText(),
              valueEdit_->text(), logicCombo_->currentText());
    valueEdit_->clear();
}

void SearchQueryBuilder::onRemove() {
    int row = clauseList_->currentRow();
    if (row < 0 || row >= clauses_.size()) return;
    clauses_.removeAt(row);
    refreshList();
    updatePreview();
}

void SearchQueryBuilder::onClear() { clearClauses(); }

void SearchQueryBuilder::onSearch() {
    if (clauses_.isEmpty()) return;
    emit searchRequested(buildQuery());
}

void SearchQueryBuilder::onClauseChanged() { updatePreview(); }

void SearchQueryBuilder::refreshList() {
    clauseList_->clear();
    for (int i = 0; i < clauses_.size(); ++i) {
        const auto& c = clauses_[i];
        QString logic = (i == 0) ? "" : QString("[%1] ").arg(c.logic);
        QString display = QString("%2%3 %4 \"%5\"")
            .arg(logic, c.field, c.operator_, c.value);
        auto* item = new QListWidgetItem(display);
        if (c.logic == "NOT") item->setForeground(QColor(220, 38, 38));
        else if (c.logic == "OR") item->setForeground(QColor(245, 158, 11));
        clauseList_->addItem(item);
    }
    countLabel_->setText(QString("%1 clause(s)").arg(clauses_.size()));
}

void SearchQueryBuilder::updatePreview() {
    previewEdit_->clear();
    previewEdit_->appendPlainText("=== Simple Query ===");
    previewEdit_->appendPlainText(buildQuery());
    previewEdit_->appendPlainText("");
    previewEdit_->appendPlainText("=== Elasticsearch DSL ===");
    previewEdit_->appendPlainText(buildElasticsearchQuery());
}
