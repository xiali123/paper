#include "latex/LatexBibtexManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>

// === BibtexEntry ===

BibtexEntry BibtexEntry::fromString(const QString& text) {
    BibtexEntry entry;
    QRegularExpression entryRe("@(\\w+)\\s*\\{\\s*([^,]+),");
    auto match = entryRe.match(text);
    if (!match.hasMatch()) return entry;

    entry.type = match.captured(1).toLower();
    entry.key = match.captured(2).trimmed();

    // Parse fields: field = {value} or field = "value" or field = number
    QRegularExpression fieldRe("(\\w+)\\s*=\\s*\\{([^}]*)\\}");
    auto iter = fieldRe.globalMatch(text);
    while (iter.hasNext()) {
        auto m = iter.next();
        entry.fields[m.captured(1).toLower()] = m.captured(2).trimmed();
    }

    // Also try "value" format
    QRegularExpression fieldQuoteRe("(\\w+)\\s*=\\s*\"([^\"]*)\"");
    iter = fieldQuoteRe.globalMatch(text);
    while (iter.hasNext()) {
        auto m = iter.next();
        QString field = m.captured(1).toLower();
        if (!entry.fields.contains(field)) {
            entry.fields[field] = m.captured(2).trimmed();
        }
    }

    return entry;
}

QString BibtexEntry::toString() const {
    QString result = QString("@%1{%2,\n").arg(type, key);
    QStringList parts;
    for (auto it = fields.constBegin(); it != fields.constEnd(); ++it) {
        parts << QString("  %1 = {%2}").arg(it.key(), it.value());
    }
    result += parts.join(",\n");
    result += "\n}\n";
    return result;
}

// === LatexBibtexManager ===

LatexBibtexManager::LatexBibtexManager(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void LatexBibtexManager::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // Search bar
    auto* searchRow = new QHBoxLayout();
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search entries...");
    searchEdit_->setClearButtonEnabled(true);
    connect(searchEdit_, &QLineEdit::textChanged, this, &LatexBibtexManager::onSearch);
    searchRow->addWidget(searchEdit_, 1);

    importBtn_ = new QPushButton("Import .bib");
    connect(importBtn_, &QPushButton::clicked, this, &LatexBibtexManager::onImport);
    searchRow->addWidget(importBtn_);

    exportBtn_ = new QPushButton("Export .bib");
    connect(exportBtn_, &QPushButton::clicked, this, &LatexBibtexManager::onExport);
    searchRow->addWidget(exportBtn_);
    layout->addLayout(searchRow);

    // Entry table
    entryTable_ = new QTableWidget();
    entryTable_->setColumnCount(5);
    entryTable_->setHorizontalHeaderLabels({"Key", "Type", "Author", "Title", "Year"});
    entryTable_->horizontalHeader()->setStretchLastSection(true);
    entryTable_->setColumnWidth(0, 120);
    entryTable_->setColumnWidth(1, 80);
    entryTable_->setColumnWidth(2, 200);
    entryTable_->setColumnWidth(4, 60);
    entryTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    entryTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    entryTable_->setAlternatingRowColors(true);
    connect(entryTable_, &QTableWidget::cellDoubleClicked, this, &LatexBibtexManager::onEntrySelected);
    connect(entryTable_, &QTableWidget::cellClicked, this, [this](int row) { selectedRow_ = row; });
    layout->addWidget(entryTable_, 1);

    // Preview
    previewEdit_ = new QTextEdit();
    previewEdit_->setReadOnly(true);
    previewEdit_->setFont(QFont("Consolas", 10));
    previewEdit_->setMaximumHeight(120);
    layout->addWidget(previewEdit_);

    // Count label
    countLabel_ = new QLabel("0 entries");
    countLabel_->setStyleSheet("color: palette(mid); font-size: 11px;");
    layout->addWidget(countLabel_);

    // Button row
    auto* btnRow = new QHBoxLayout();

    addBtn_ = new QPushButton("Add");
    connect(addBtn_, &QPushButton::clicked, this, &LatexBibtexManager::onAddEntry);
    btnRow->addWidget(addBtn_);

    editBtn_ = new QPushButton("Edit");
    connect(editBtn_, &QPushButton::clicked, this, &LatexBibtexManager::onEditEntry);
    btnRow->addWidget(editBtn_);

    deleteBtn_ = new QPushButton("Delete");
    connect(deleteBtn_, &QPushButton::clicked, this, &LatexBibtexManager::onDeleteEntry);
    btnRow->addWidget(deleteBtn_);

    btnRow->addStretch();

    citeBtn_ = new QPushButton("Cite");
    citeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 4px; padding: 4px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(citeBtn_, &QPushButton::clicked, this, &LatexBibtexManager::onInsertCite);
    btnRow->addWidget(citeBtn_);

    layout->addLayout(btnRow);
}

void LatexBibtexManager::loadBibtex(const QString& content) {
    entries_.clear();
    QRegularExpression entryRe("@(\\w+)\\s*\\{");
    auto iter = entryRe.globalMatch(content);
    QList<int> starts;
    while (iter.hasNext()) {
        starts << iter.next().capturedStart();
    }

    for (int i = 0; i < starts.size(); ++i) {
        int end = (i + 1 < starts.size()) ? starts[i + 1] : content.size();
        QString entryText = content.mid(starts[i], end - starts[i]);
        BibtexEntry entry = BibtexEntry::fromString(entryText);
        if (!entry.key.isEmpty()) {
            entries_.append(entry);
        }
    }

    refreshTable();
}

QString LatexBibtexManager::exportBibtex() const {
    QString result;
    for (const auto& entry : entries_) {
        result += entry.toString() + "\n";
    }
    return result;
}

void LatexBibtexManager::refreshTable() {
    entryTable_->setRowCount(entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        entryTable_->setItem(i, 0, new QTableWidgetItem(e.key));
        entryTable_->setItem(i, 1, new QTableWidgetItem(e.type));
        entryTable_->setItem(i, 2, new QTableWidgetItem(e.fields.value("author", "")));
        entryTable_->setItem(i, 3, new QTableWidgetItem(e.fields.value("title", "")));
        entryTable_->setItem(i, 4, new QTableWidgetItem(e.fields.value("year", "")));
    }
    countLabel_->setText(QString("%1 entries").arg(entries_.size()));
}

void LatexBibtexManager::showEditDialog(BibtexEntry& entry) {
    QDialog dlg(this);
    dlg.setWindowTitle(entry.key.isEmpty() ? "Add Entry" : "Edit Entry");
    dlg.setMinimumWidth(500);
    auto* form = new QFormLayout(&dlg);

    auto* typeCombo = new QComboBox();
    typeCombo->addItems({"article", "book", "inproceedings", "incollection",
                         "phdthesis", "mastersthesis", "techreport", "misc"});
    int typeIdx = typeCombo->findText(entry.type);
    if (typeIdx >= 0) typeCombo->setCurrentIndex(typeIdx);
    form->addRow("Type:", typeCombo);

    auto* keyEdit = new QLineEdit(entry.key);
    form->addRow("Citation Key:", keyEdit);

    // Common fields
    QMap<QString, QString> fieldDefaults = {
        {"author", ""}, {"title", ""}, {"year", ""}, {"journal", ""},
        {"booktitle", ""}, {"publisher", ""}, {"volume", ""}, {"number", ""},
        {"pages", ""}, {"doi", ""}, {"url", ""}, {"abstract", ""},
    };

    QMap<QString, QLineEdit*> fieldEdits;
    for (auto it = fieldDefaults.constBegin(); it != fieldDefaults.constEnd(); ++it) {
        auto* edit = new QLineEdit(entry.fields.value(it.key(), it.value()));
        fieldEdits[it.key()] = edit;
        form->addRow(it.key() + ":", edit);
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        entry.type = typeCombo->currentText();
        entry.key = keyEdit->text().trimmed();
        for (auto it = fieldEdits.constBegin(); it != fieldEdits.constEnd(); ++it) {
            QString val = it.value()->text().trimmed();
            if (!val.isEmpty()) {
                entry.fields[it.key()] = val;
            } else {
                entry.fields.remove(it.key());
            }
        }
    }
}

void LatexBibtexManager::onAddEntry() {
    BibtexEntry entry;
    showEditDialog(entry);
    if (!entry.key.isEmpty()) {
        entries_.append(entry);
        refreshTable();
        emit bibtexChanged(exportBibtex());
    }
}

void LatexBibtexManager::onEditEntry() {
    if (selectedRow_ < 0 || selectedRow_ >= entries_.size()) return;
    showEditDialog(entries_[selectedRow_]);
    refreshTable();
    emit bibtexChanged(exportBibtex());
}

void LatexBibtexManager::onDeleteEntry() {
    if (selectedRow_ < 0 || selectedRow_ >= entries_.size()) return;
    auto result = QMessageBox::question(this, "Delete",
        QString("Delete entry '%1'?").arg(entries_[selectedRow_].key));
    if (result == QMessageBox::Yes) {
        entries_.removeAt(selectedRow_);
        selectedRow_ = -1;
        refreshTable();
        emit bibtexChanged(exportBibtex());
    }
}

void LatexBibtexManager::onImport() {
    QString path = QFileDialog::getOpenFileName(this, "Import BibTeX", "",
        "BibTeX Files (*.bib);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot open file.");
        return;
    }
    QTextStream in(&file);
    loadBibtex(in.readAll());
    emit bibtexChanged(exportBibtex());
}

void LatexBibtexManager::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export BibTeX", "",
        "BibTeX Files (*.bib);;All Files (*)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "Cannot write file.");
        return;
    }
    QTextStream out(&file);
    out << exportBibtex();
}

void LatexBibtexManager::onInsertCite() {
    if (selectedRow_ < 0 || selectedRow_ >= entries_.size()) return;
    emit insertCitation(entries_[selectedRow_].key);
}

void LatexBibtexManager::onSearch(const QString& text) {
    QString lower = text.trimmed().toLower();
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        bool match = lower.isEmpty() ||
                     e.key.toLower().contains(lower) ||
                     e.fields.value("title", "").toLower().contains(lower) ||
                     e.fields.value("author", "").toLower().contains(lower);
        entryTable_->setRowHidden(i, !match);
    }
}

void LatexBibtexManager::onEntrySelected(int row, int) {
    if (row < 0 || row >= entries_.size()) return;
    selectedRow_ = row;
    previewEdit_->setPlainText(entries_[row].toString());
}
