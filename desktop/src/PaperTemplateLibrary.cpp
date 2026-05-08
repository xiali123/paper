#include "PaperTemplateLibrary.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QInputDialog>
#include <QMessageBox>

PaperTemplateLibrary::PaperTemplateLibrary(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadDefaults();
    loadSettings();
    refreshTree();
}

void PaperTemplateLibrary::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Search bar
    auto* searchRow = new QHBoxLayout();
    searchRow->addWidget(new QLabel("Search:"));
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search templates...");
    connect(searchEdit_, &QLineEdit::textChanged, this, &PaperTemplateLibrary::onSearchChanged);
    searchRow->addWidget(searchEdit_, 1);

    searchRow->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Article", "Conference", "Thesis", "Review", "Note", "Custom"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTemplateLibrary::onCategoryChanged);
    searchRow->addWidget(categoryCombo_);
    layout->addLayout(searchRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Template list
    categoryTree_ = new QTreeWidget();
    categoryTree_->setHeaderLabels({"Template", "Category", "Tags"});
    categoryTree_->setColumnWidth(0, 200);
    categoryTree_->setColumnWidth(1, 80);
    categoryTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(categoryTree_, &QTreeWidget::itemClicked, this, &PaperTemplateLibrary::onTemplateSelected);
    splitter->addWidget(categoryTree_);

    // Preview
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addWidget(new QLabel("Preview:"));
    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setStyleSheet("QTextEdit { border: 1px solid palette(mid); border-radius: 4px; font-family: monospace; }");
    rightLayout->addWidget(previewEdit_, 1);
    splitter->addWidget(rightPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    // Create/Edit form
    auto* formRow = new QHBoxLayout();
    formRow->addWidget(new QLabel("Name:"));
    nameEdit_ = new QLineEdit();
    nameEdit_->setPlaceholderText("Template name...");
    formRow->addWidget(nameEdit_, 1);

    formRow->addWidget(new QLabel("Desc:"));
    descEdit_ = new QTextEdit();
    descEdit_->setMaximumHeight(40);
    descEdit_->setPlaceholderText("Description...");
    formRow->addWidget(descEdit_, 1);
    layout->addLayout(formRow);

    contentEdit_ = new QTextEdit();
    contentEdit_->setMaximumHeight(80);
    contentEdit_->setPlaceholderText("Template content (LaTeX, markdown, or notes)...");
    layout->addWidget(contentEdit_);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    applyBtn_ = new QPushButton("Apply");
    applyBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(applyBtn_, &QPushButton::clicked, this, &PaperTemplateLibrary::onApply);
    btnRow->addWidget(applyBtn_);

    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperTemplateLibrary::onCreate);
    btnRow->addWidget(createBtn_);

    editBtn_ = new QPushButton("Edit");
    connect(editBtn_, &QPushButton::clicked, this, &PaperTemplateLibrary::onEdit);
    btnRow->addWidget(editBtn_);

    duplicateBtn_ = new QPushButton("Duplicate");
    connect(duplicateBtn_, &QPushButton::clicked, this, &PaperTemplateLibrary::onDuplicate);
    btnRow->addWidget(duplicateBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperTemplateLibrary::onDelete);
    btnRow->addWidget(deleteBtn_);

    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 templates");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperTemplateLibrary::addTemplate(const PaperTemplate& tmpl) {
    PaperTemplate t = tmpl;
    if (t.id < 0) t.id = nextId_++;
    templates_.append(t);
    nextId_ = qMax(nextId_, t.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
}

void PaperTemplateLibrary::removeTemplate(int templateId) {
    templates_.removeIf([templateId](const PaperTemplate& t) { return t.id == templateId; });
    refreshTree();
    saveSettings();
    updateStats();
    emit templateDeleted(templateId);
}

QList<PaperTemplate> PaperTemplateLibrary::templates() const { return templates_; }

QList<PaperTemplate> PaperTemplateLibrary::templatesByCategory(const QString& category) const {
    QList<PaperTemplate> result;
    for (const auto& t : templates_) {
        if (t.category == category) result.append(t);
    }
    return result;
}

PaperTemplate PaperTemplateLibrary::templateById(int id) const {
    for (const auto& t : templates_) {
        if (t.id == id) return t;
    }
    return PaperTemplate{};
}

void PaperTemplateLibrary::onCategoryChanged(int) { refreshTree(); }

void PaperTemplateLibrary::onTemplateSelected() {
    auto* item = categoryTree_->currentItem();
    if (!item) return;
    selectedId_ = item->data(0, Qt::UserRole).toInt();
    updatePreview();
}

void PaperTemplateLibrary::onApply() {
    if (selectedId_ < 0) return;
    for (const auto& t : templates_) {
        if (t.id == selectedId_) {
            emit templateApplied(t.id, t.content);
            break;
        }
    }
}

void PaperTemplateLibrary::onCreate() {
    QString name = nameEdit_->text().trimmed();
    if (name.isEmpty()) return;

    PaperTemplate t;
    t.name = name;
    t.category = categoryCombo_->currentText() == "All" ? "Custom" : categoryCombo_->currentText();
    t.description = descEdit_->toPlainText();
    t.content = contentEdit_->toPlainText();
    t.builtin = false;
    addTemplate(t);

    nameEdit_->clear();
    descEdit_->clear();
    contentEdit_->clear();

    emit templateCreated(t.id);
}

void PaperTemplateLibrary::onEdit() {
    if (selectedId_ < 0) return;
    for (auto& t : templates_) {
        if (t.id == selectedId_) {
            if (t.builtin) {
                // Builtin templates: edit via duplicate
                onDuplicate();
                return;
            }
            if (!nameEdit_->text().trimmed().isEmpty()) t.name = nameEdit_->text().trimmed();
            if (!descEdit_->toPlainText().isEmpty()) t.description = descEdit_->toPlainText();
            if (!contentEdit_->toPlainText().isEmpty()) t.content = contentEdit_->toPlainText();
            refreshTree();
            saveSettings();
            updatePreview();
            break;
        }
    }
}

void PaperTemplateLibrary::onDelete() {
    if (selectedId_ < 0) return;
    for (const auto& t : templates_) {
        if (t.id == selectedId_ && t.builtin) return;
    }
    removeTemplate(selectedId_);
    selectedId_ = -1;
    previewEdit_->clear();
}

void PaperTemplateLibrary::onDuplicate() {
    if (selectedId_ < 0) return;
    for (const auto& t : templates_) {
        if (t.id == selectedId_) {
            PaperTemplate dup = t;
            dup.id = -1;
            dup.name = t.name + " (Copy)";
            dup.builtin = false;
            addTemplate(dup);
            break;
        }
    }
}

void PaperTemplateLibrary::onSearchChanged(const QString&) { refreshTree(); }

void PaperTemplateLibrary::loadDefaults() {
    QList<PaperTemplate> defaults = {
        {-1, "Research Article", "Article", "Standard research paper structure",
         "\\title{}\n\\author{}\n\\begin{abstract}\n\n\\end{abstract}\n\\section{Introduction}\n\\section{Related Work}\n\\section{Method}\n\\section{Experiments}\n\\section{Conclusion}\n\\bibliography{refs}", {}, true},
        {-1, "Conference Paper", "Conference", "Short conference paper",
         "\\title{}\n\\author{}\n\\begin{abstract}\n\n\\end{abstract}\n\\section{Introduction}\n\\section{Approach}\n\\section{Results}\n\\section{Discussion}\n", {}, true},
        {-1, "Literature Review", "Review", "Systematic literature review",
         "\\title{Literature Review}\n\\section{Introduction}\n\\section{Search Strategy}\n\\section{Included Studies}\n\\section{Analysis}\n\\section{Gaps and Future Directions}\n\\section{Conclusion}\n", {}, true},
        {-1, "Reading Note", "Note", "Single paper reading notes",
         "# Paper: \n## Summary\n\n## Key Contributions\n\n## Methodology\n\n## Strengths\n\n## Weaknesses\n\n## Questions\n\n## Related Papers\n", {}, true},
        {-1, "Comparison Note", "Note", "Multi-paper comparison template",
         "# Paper Comparison\n## Criteria\n| | Paper A | Paper B | Paper C |\n|---|---|---|---|\n| Method | | | |\n| Dataset | | | |\n| Performance | | | |\n| Year | | | |\n\n## Analysis\n", {}, true},
        {-1, "Thesis Chapter", "Thesis", "Thesis chapter structure",
         "\\chapter{}\n\\section{Introduction}\n\\section{Background}\n\\section{Contribution}\n\\section{Evaluation}\n\\section{Summary}\n", {}, true},
    };
    for (auto& d : defaults) addTemplate(d);
}

void PaperTemplateLibrary::refreshTree() {
    categoryTree_->clear();
    QString search = searchEdit_->text().trimmed().toLower();
    QString catFilter = categoryCombo_->currentText();

    for (const auto& t : templates_) {
        if (catFilter != "All" && t.category != catFilter) continue;
        if (!search.isEmpty()) {
            bool match = t.name.toLower().contains(search)
                         || t.description.toLower().contains(search)
                         || t.content.toLower().contains(search);
            if (!match) continue;
        }

        auto* item = new QTreeWidgetItem({
            t.name,
            t.category,
            t.tags.join(", ")
        });
        item->setData(0, Qt::UserRole, t.id);
        if (t.builtin) {
            QFont f = item->font(0);
            f.setBold(true);
            item->setFont(0, f);
        }
        categoryTree_->addTopLevelItem(item);
    }
    updateStats();
}

void PaperTemplateLibrary::updatePreview() {
    if (selectedId_ < 0) { previewEdit_->clear(); return; }
    for (const auto& t : templates_) {
        if (t.id == selectedId_) {
            QString html = QString("<h3>%1</h3><p><i>%2</i></p><pre>%3</pre>")
                .arg(t.name, t.description, t.content.toHtmlEscaped());
            previewEdit_->setHtml(html);
            break;
        }
    }
}

void PaperTemplateLibrary::populateCategories() {
    QSet<QString> cats;
    for (const auto& t : templates_) cats.insert(t.category);
    // categoryCombo_ already has defaults
}

void PaperTemplateLibrary::loadSettings() {
    QSettings settings("PaperCrawler", "TemplateLibrary");
    QByteArray data = settings.value("custom_templates").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        if (obj["builtin"].toBool()) continue;
        PaperTemplate t;
        t.id = obj["id"].toInt();
        t.name = obj["name"].toString();
        t.category = obj["category"].toString();
        t.description = obj["description"].toString();
        t.content = obj["content"].toString();
        t.builtin = false;
        templates_.append(t);
        nextId_ = qMax(nextId_, t.id + 1);
    }
    refreshTree();
}

void PaperTemplateLibrary::saveSettings() {
    QJsonArray arr;
    for (const auto& t : templates_) {
        if (t.builtin) continue;
        QJsonObject obj;
        obj["id"] = t.id;
        obj["name"] = t.name;
        obj["category"] = t.category;
        obj["description"] = t.description;
        obj["content"] = t.content;
        obj["builtin"] = false;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "TemplateLibrary");
    settings.setValue("custom_templates", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void PaperTemplateLibrary::updateStats() {
    int total = templates_.size();
    int builtin = 0;
    for (const auto& t : templates_) if (t.builtin) builtin++;
    statsLabel_->setText(QString("%1 templates (%2 builtin, %3 custom)").arg(total).arg(builtin).arg(total - builtin));
}
