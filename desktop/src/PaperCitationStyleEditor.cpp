#include "PaperCitationStyleEditor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QInputDialog>
#include <QApplication>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PaperCitationStyleEditor::PaperCitationStyleEditor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadDefaults();
    loadSettings();
}

void PaperCitationStyleEditor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Style selector
    auto* styleRow = new QHBoxLayout();
    styleRow->addWidget(new QLabel("Style:"));
    styleCombo_ = new QComboBox();
    connect(styleCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCitationStyleEditor::onStyleChanged);
    styleRow->addWidget(styleCombo_, 1);

    formatBtn_ = new QPushButton("Format");
    formatBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(formatBtn_, &QPushButton::clicked, this, &PaperCitationStyleEditor::onFormat);
    styleRow->addWidget(formatBtn_);

    copyBtn_ = new QPushButton("Copy");
    connect(copyBtn_, &QPushButton::clicked, this, &PaperCitationStyleEditor::onCopy);
    styleRow->addWidget(copyBtn_);
    layout->addLayout(styleRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: entry form
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    auto makeField = [&](const QString& label, QLineEdit*& edit) {
        auto* row = new QHBoxLayout();
        row->addWidget(new QLabel(label));
        edit = new QLineEdit();
        edit->setPlaceholderText(label + "...");
        row->addWidget(edit, 1);
        leftLayout->addLayout(row);
    };

    makeField("Authors:", authorsEdit_);
    makeField("Title:", titleEdit_);
    makeField("Year:", yearEdit_);
    makeField("Journal:", journalEdit_);
    makeField("Volume:", volumeEdit_);
    makeField("Pages:", pagesEdit_);
    makeField("DOI:", doiEdit_);
    leftLayout->addStretch();
    splitter->addWidget(leftPanel);

    // Right: template + preview
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    rightLayout->addWidget(new QLabel("Template:"));
    templateEdit_ = new QTextEdit();
    templateEdit_->setMaximumHeight(50);
    templateEdit_->setPlaceholderText("Template: {authors} ({year}). {title}. {journal}, {volume}, {pages}.");
    rightLayout->addWidget(templateEdit_);

    previewBtn_ = new QPushButton("Preview");
    connect(previewBtn_, &QPushButton::clicked, this, &PaperCitationStyleEditor::onPreview);
    rightLayout->addWidget(previewBtn_);

    rightLayout->addWidget(new QLabel("Preview:"));
    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setStyleSheet("QTextEdit { border: 1px solid palette(mid); border-radius: 4px; background: #f8fafc; font-size: 12px; }");
    rightLayout->addWidget(previewEdit_, 1);

    auto* btnRow = new QHBoxLayout();
    createBtn_ = new QPushButton("Create Style");
    createBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 3px 10px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperCitationStyleEditor::onCreateStyle);
    btnRow->addWidget(createBtn_);

    deleteBtn_ = new QPushButton("Delete Style");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperCitationStyleEditor::onDeleteStyle);
    btnRow->addWidget(deleteBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 styles");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperCitationStyleEditor::addStyle(const CitationStyle& style) {
    for (auto& s : styles_) {
        if (s.name == style.name) { s = style; return; }
    }
    styles_.append(style);
    styleCombo_->addItem(style.name);
    statsLabel_->setText(QString("%1 styles").arg(styles_.size()));
}

void PaperCitationStyleEditor::removeStyle(const QString& name) {
    styles_.removeIf([&name](const CitationStyle& s) { return s.name == name && !s.builtin; });
    styleCombo_->clear();
    for (const auto& s : styles_) styleCombo_->addItem(s.name);
    saveSettings();
}

QList<CitationStyle> PaperCitationStyleEditor::styles() const { return styles_; }

QString PaperCitationStyleEditor::formatCitation(const CitationEntry& entry, const QString& styleName) const {
    QString tmpl;
    for (const auto& s : styles_) {
        if (s.name == styleName) { tmpl = s.templateStr; break; }
    }
    if (tmpl.isEmpty()) tmpl = "{authors} ({year}). {title}. {journal}, {volume}, {pages}.";

    tmpl.replace("{authors}", entry.authors);
    tmpl.replace("{title}", entry.title);
    tmpl.replace("{year}", entry.year);
    tmpl.replace("{journal}", entry.journal);
    tmpl.replace("{volume}", entry.volume);
    tmpl.replace("{pages}", entry.pages);
    tmpl.replace("{doi}", entry.doi.isEmpty() ? "" : "https://doi.org/" + entry.doi);

    // Clean up empty placeholders
    tmpl.replace(", ,", ",");
    tmpl.replace(", .", ".");
    tmpl.replace("  ", " ");
    return tmpl.trimmed();
}

void PaperCitationStyleEditor::onStyleChanged(int index) {
    if (index < 0 || index >= styles_.size()) return;
    templateEdit_->setPlainText(styles_[index].templateStr);
    refreshPreview();
}

void PaperCitationStyleEditor::onFormat() {
    CitationEntry entry;
    entry.authors = authorsEdit_->text();
    entry.title = titleEdit_->text();
    entry.year = yearEdit_->text();
    entry.journal = journalEdit_->text();
    entry.volume = volumeEdit_->text();
    entry.pages = pagesEdit_->text();
    entry.doi = doiEdit_->text();

    QString style = styleCombo_->currentText();
    QString formatted = formatCitation(entry, style);
    previewEdit_->setPlainText(formatted);
    emit styleApplied(style, formatted);
}

void PaperCitationStyleEditor::onCopy() {
    QString text = previewEdit_->toPlainText();
    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
        emit citationCopied(text);
    }
}

void PaperCitationStyleEditor::onCreateStyle() {
    bool ok;
    QString name = QInputDialog::getText(this, "Create Style", "Style name:", QLineEdit::Normal, "", &ok);
    if (!ok || name.trimmed().isEmpty()) return;
    CitationStyle s;
    s.name = name.trimmed();
    s.templateStr = templateEdit_->toPlainText();
    s.description = "Custom style";
    s.builtin = false;
    addStyle(s);
    saveSettings();
}

void PaperCitationStyleEditor::onDeleteStyle() {
    QString name = styleCombo_->currentText();
    removeStyle(name);
}

void PaperCitationStyleEditor::onPreview() { refreshPreview(); }

void PaperCitationStyleEditor::refreshPreview() {
    CitationEntry entry;
    entry.authors = authorsEdit_->text().isEmpty() ? "Smith, J." : authorsEdit_->text();
    entry.title = titleEdit_->text().isEmpty() ? "A Study of Methods" : titleEdit_->text();
    entry.year = yearEdit_->text().isEmpty() ? "2024" : yearEdit_->text();
    entry.journal = journalEdit_->text().isEmpty() ? "Nature" : journalEdit_->text();
    entry.volume = volumeEdit_->text().isEmpty() ? "123" : volumeEdit_->text();
    entry.pages = pagesEdit_->text().isEmpty() ? "45-67" : pagesEdit_->text();
    entry.doi = doiEdit_->text();

    QString formatted = formatCitation(entry, styleCombo_->currentText());
    previewEdit_->setPlainText(formatted);
}

void PaperCitationStyleEditor::loadDefaults() {
    QList<CitationStyle> defaults = {
        {"APA", "{authors} ({year}). {title}. {journal}, {volume}, {pages}.", "APA 7th Edition", true},
        {"MLA", "{authors}. \"{title}.\" {journal} {volume} ({year}): {pages}.", "MLA 9th Edition", true},
        {"Chicago", "{authors}. \"{title}.\" {journal} {volume}, no. ({year}): {pages}.", "Chicago", true},
        {"IEEE", "[1] {authors}, \"{title},\" {journal}, vol. {volume}, pp. {pages}, {year}.", "IEEE", true},
        {"Vancouver", "{authors}. {title}. {journal}. {year};{volume}:{pages}.", "Vancouver", true},
        {"Harvard", "{authors} ({year}) '{title}', {journal}, {volume}, pp. {pages}.", "Harvard", true},
        {"BibTeX", "@article{key, author={{authors}}, title={{title}}, journal={{journal}}, volume={{{volume}}}, pages={{{pages}}}, year={{{year}}}}", "BibTeX", true},
    };
    for (const auto& d : defaults) addStyle(d);
}

void PaperCitationStyleEditor::loadSettings() {
    QSettings settings("PaperCrawler", "CitationStyles");
    QByteArray data = settings.value("custom").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        CitationStyle s;
        s.name = obj["name"].toString();
        s.templateStr = obj["template"].toString();
        s.description = obj["description"].toString();
        s.builtin = false;
        addStyle(s);
    }
    if (styleCombo_->count() > 0) styleCombo_->setCurrentIndex(0);
}

void PaperCitationStyleEditor::saveSettings() {
    QJsonArray arr;
    for (const auto& s : styles_) {
        if (s.builtin) continue;
        QJsonObject obj;
        obj["name"] = s.name;
        obj["template"] = s.templateStr;
        obj["description"] = s.description;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "CitationStyles");
    settings.setValue("custom", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
