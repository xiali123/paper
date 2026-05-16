#include "workspace/ExportTemplateManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHeaderView>

ExportTemplateManager::ExportTemplateManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadDefaults();
    loadSettings();
}

void ExportTemplateManager::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left: template tree
    auto* leftPanel = new QVBoxLayout();
    auto* header = new QLabel("Export Templates");
    header->setStyleSheet("font-weight: bold; font-size: 13px;");
    leftPanel->addWidget(header);

    templateTree_ = new QTreeWidget();
    templateTree_->setHeaderLabels({"Name", "Format"});
    templateTree_->setColumnWidth(0, 150);
    templateTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(templateTree_, &QTreeWidget::itemClicked, this, &ExportTemplateManager::onItemSelected);
    leftPanel->addWidget(templateTree_, 1);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("New");
    addBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(addBtn_, &QPushButton::clicked, this, &ExportTemplateManager::onAdd);
    btnRow->addWidget(addBtn_);

    auto* dupBtn = new QPushButton("Duplicate");
    connect(dupBtn, &QPushButton::clicked, this, &ExportTemplateManager::onDuplicate);
    btnRow->addWidget(dupBtn);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &ExportTemplateManager::onDelete);
    btnRow->addWidget(deleteBtn_);

    leftPanel->addLayout(btnRow);

    infoLabel_ = new QLabel("");
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    leftPanel->addWidget(infoLabel_);

    mainLayout->addLayout(leftPanel, 1);

    // Right: template editor + preview
    auto* rightPanel = new QVBoxLayout();

    rightPanel->addWidget(new QLabel("Template:"));
    templateEdit_ = new QPlainTextEdit();
    templateEdit_->setFont(QFont("Consolas", 10));
    templateEdit_->setPlaceholderText("{{title}}\t{{authors}}\t{{year}}\t{{journal}}");
    rightPanel->addWidget(templateEdit_, 1);

    auto* previewBtn = new QPushButton("Preview");
    connect(previewBtn, &QPushButton::clicked, this, &ExportTemplateManager::onPreview);
    rightPanel->addWidget(previewBtn);

    rightPanel->addWidget(new QLabel("Preview:"));
    previewEdit_ = new QPlainTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Consolas", 10));
    previewEdit_->setMaximumHeight(150);
    rightPanel->addWidget(previewEdit_);

    mainLayout->addLayout(rightPanel, 1);
}

void ExportTemplateManager::setTemplates(const QList<ExportTemplate>& tmpl) {
    templates_ = tmpl;
    refreshTree();
}

QList<ExportTemplate> ExportTemplateManager::templates() const {
    return templates_;
}

void ExportTemplateManager::addTemplate(const ExportTemplate& tmpl) {
    templates_.append(tmpl);
    nextId_ = qMax(nextId_, tmpl.id + 1);
    refreshTree();
    saveSettings();
    emit templateCreated(tmpl);
}

void ExportTemplateManager::removeTemplate(int templateId) {
    templates_.removeIf([templateId](const ExportTemplate& t) { return t.id == templateId; });
    refreshTree();
    saveSettings();
    emit templateDeleted(templateId);
}

void ExportTemplateManager::duplicateTemplate(int templateId) {
    for (const auto& t : templates_) {
        if (t.id == templateId) {
            ExportTemplate dup = t;
            dup.id = nextId_++;
            dup.name = t.name + " (Copy)";
            dup.builtin = false;
            addTemplate(dup);
            return;
        }
    }
}

void ExportTemplateManager::loadDefaults() {
    QList<ExportTemplate> defaults = {
        {nextId_++, "Standard CSV", "csv", "{{title}},\"{{authors}}\",{{year}},{{journal}}", "Basic CSV export", true},
        {nextId_++, "Full CSV", "csv", "{{id}},{{title}},\"{{authors}}\",{{year}},{{journal}},{{doi}},{{abstract}}", "All fields CSV", true},
        {nextId_++, "Markdown Table", "markdown", "| Title | Authors | Year |\n|---|---|---|\n| {{title}} | {{authors}} | {{year}} |", "Markdown table format", true},
        {nextId_++, "BibTeX Entry", "bibtex", "@article{{{key}},\n  title={{{title}}},\n  author={{{authors}}},\n  year={{{year}}}\n}", "BibTeX format", true},
        {nextId_++, "JSON Object", "json", "{\"title\": \"{{title}}\", \"authors\": \"{{authors}}\", \"year\": \"{{year}}\"}", "JSON per paper", true},
    };
    for (const auto& d : defaults) templates_.append(d);
    refreshTree();
}

void ExportTemplateManager::loadSettings() {
    QSettings settings("PaperCrawler", "ExportTemplates");
    QByteArray data = settings.value("custom_templates").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        ExportTemplate t;
        t.id = obj["id"].toInt();
        t.name = obj["name"].toString();
        t.format = obj["format"].toString();
        t.template_ = obj["template"].toString();
        t.description = obj["description"].toString();
        t.builtin = false;
        templates_.append(t);
        nextId_ = qMax(nextId_, t.id + 1);
    }
    refreshTree();
}

void ExportTemplateManager::saveSettings() {
    QJsonArray arr;
    for (const auto& t : templates_) {
        if (t.builtin) continue;
        QJsonObject obj;
        obj["id"] = t.id;
        obj["name"] = t.name;
        obj["format"] = t.format;
        obj["template"] = t.template_;
        obj["description"] = t.description;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ExportTemplates");
    settings.setValue("custom_templates", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void ExportTemplateManager::onAdd() {
    auto* dlg = new QDialog(this);
    dlg->setWindowTitle("New Template");
    auto* layout = new QFormLayout(dlg);

    auto* nameEdit = new QLineEdit();
    layout->addRow("Name:", nameEdit);

    auto* formatCombo = new QComboBox();
    formatCombo->addItems({"csv", "bibtex", "markdown", "json", "custom"});
    layout->addRow("Format:", formatCombo);

    auto* tmplEdit = new QPlainTextEdit();
    tmplEdit->setPlaceholderText("{{title}}, {{authors}}, {{year}}");
    layout->addRow("Template:", tmplEdit);

    auto* btnRow = new QHBoxLayout();
    auto* okBtn = new QPushButton("Create");
    auto* cancelBtn = new QPushButton("Cancel");
    connect(okBtn, &QPushButton::clicked, dlg, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, dlg, &QDialog::reject);
    btnRow->addWidget(okBtn);
    btnRow->addWidget(cancelBtn);
    layout->addRow(btnRow);

    if (dlg->exec() == QDialog::Accepted) {
        ExportTemplate t;
        t.id = nextId_++;
        t.name = nameEdit->text().trimmed();
        t.format = formatCombo->currentText();
        t.template_ = tmplEdit->toPlainText();
        t.builtin = false;
        addTemplate(t);
    }
    dlg->deleteLater();
}

void ExportTemplateManager::onDelete() {
    if (selectedId_ < 0) return;
    for (const auto& t : templates_) {
        if (t.id == selectedId_ && t.builtin) {
            QMessageBox::warning(this, "Cannot Delete", "Built-in templates cannot be deleted.");
            return;
        }
    }
    removeTemplate(selectedId_);
    selectedId_ = -1;
}

void ExportTemplateManager::onDuplicate() {
    if (selectedId_ < 0) return;
    duplicateTemplate(selectedId_);
}

void ExportTemplateManager::onItemSelected(QTreeWidgetItem* item, int) {
    if (!item) return;
    selectedId_ = item->data(0, Qt::UserRole).toInt();
    for (const auto& t : templates_) {
        if (t.id == selectedId_) {
            templateEdit_->setPlainText(t.template_);
            infoLabel_->setText(QString("%1 | %2 | %3")
                .arg(t.name, t.format, t.builtin ? "Built-in" : "Custom"));
            emit templateSelected(t);
            break;
        }
    }
}

void ExportTemplateManager::onPreview() {
    QString tmpl = templateEdit_->toPlainText();
    QString preview = tmpl;
    preview.replace("{{title}}", "Sample Paper Title");
    preview.replace("{{authors}}", "Smith, J.; Doe, A.");
    preview.replace("{{year}}", "2024");
    preview.replace("{{journal}}", "Nature");
    preview.replace("{{doi}}", "10.1038/sample");
    preview.replace("{{abstract}}", "Sample abstract text...");
    preview.replace("{{id}}", "42");
    preview.replace("{{key}}", "smith2024sample");
    previewEdit_->setPlainText(preview);
}

void ExportTemplateManager::refreshTree() {
    templateTree_->clear();
    QMap<QString, QList<ExportTemplate>> grouped;
    for (const auto& t : templates_) {
        grouped[t.format.isEmpty() ? "custom" : t.format].append(t);
    }
    for (auto it = grouped.constBegin(); it != grouped.constEnd(); ++it) {
        auto* cat = new QTreeWidgetItem({it.key().toUpper(), ""});
        QFont font;
        font.setBold(true);
        cat->setFont(0, font);
        templateTree_->addTopLevelItem(cat);
        for (const auto& t : it.value()) {
            auto* item = new QTreeWidgetItem({t.name, t.format});
            item->setData(0, Qt::UserRole, t.id);
            if (t.builtin) item->setForeground(0, QColor(100, 116, 139));
            cat->addChild(item);
        }
    }
}
