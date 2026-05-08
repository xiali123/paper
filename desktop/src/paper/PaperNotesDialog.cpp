#include "paper/PaperNotesDialog.hpp"
#include "core/ApiManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSplitter>
#include <QMessageBox>
#include <QInputDialog>
#include <QDateTime>
#include <QDialogButtonBox>

PaperNote PaperNote::fromJson(const QJsonObject& json) {
    PaperNote n;
    n.id = json["id"].toInt();
    n.paperId = json["paperId"].toInt(json["paper_id"].toInt());
    n.content = json["content"].toString();
    n.highlight = json["highlight"].toString(json["quote"].toString());
    n.page = json["page"].toInt();
    n.createdAt = json["createdAt"].toString(json["created_at"].toString());
    return n;
}

QJsonObject PaperNote::toJson() const {
    QJsonObject j;
    j["id"] = id;
    j["paperId"] = paperId;
    j["content"] = content;
    j["highlight"] = highlight;
    j["page"] = page;
    return j;
}

PaperNotesDialog::PaperNotesDialog(int paperId, const QString& paperTitle,
                                     ApiManager* apiManager, QWidget* parent)
    : QDialog(parent)
    , paperId_(paperId)
    , paperTitle_(paperTitle)
    , apiManager_(apiManager)
{
    setWindowTitle(QString("Notes — %1").arg(paperTitle));
    resize(700, 500);
    setupUI();
    loadNotes();
}

void PaperNotesDialog::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Info header
    infoLabel_ = new QLabel(QString("Paper: %1 (ID: %2)").arg(paperTitle_).arg(paperId_));
    infoLabel_->setStyleSheet("font-weight: bold; font-size: 12px; color: palette(text); padding: 4px;");
    layout->addWidget(infoLabel_);

    // Splitter: list | editor
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: notes list
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    notesList_ = new QListWidget();
    notesList_->setMaximumWidth(250);
    connect(notesList_, &QListWidget::currentRowChanged, this, &PaperNotesDialog::onNoteSelected);
    leftLayout->addWidget(notesList_);

    auto* listBtnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("+");
    addBtn_->setToolTip("Add note");
    connect(addBtn_, &QPushButton::clicked, this, &PaperNotesDialog::onAddNote);
    listBtnRow->addWidget(addBtn_);

    editBtn_ = new QPushButton("Edit");
    editBtn_->setEnabled(false);
    connect(editBtn_, &QPushButton::clicked, this, &PaperNotesDialog::onEditNote);
    listBtnRow->addWidget(editBtn_);

    deleteBtn_ = new QPushButton("Del");
    deleteBtn_->setEnabled(false);
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperNotesDialog::onDeleteNote);
    listBtnRow->addWidget(deleteBtn_);
    leftLayout->addLayout(listBtnRow);

    splitter->addWidget(leftPanel);

    // Right: note editor
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(4, 0, 0, 0);

    auto* highlightRow = new QHBoxLayout();
    auto* hlLabel = new QLabel("Highlight:");
    hlLabel->setStyleSheet("font-weight: bold; font-size: 11px;");
    highlightRow->addWidget(hlLabel);
    highlightEdit_ = new QLineEdit();
    highlightEdit_->setPlaceholderText("Quoted text from paper...");
    highlightEdit_->setReadOnly(true);
    highlightRow->addWidget(highlightEdit_, 1);
    rightLayout->addLayout(highlightRow);

    auto* pageRow = new QHBoxLayout();
    auto* pageLabel = new QLabel("Page:");
    pageLabel->setStyleSheet("font-weight: bold; font-size: 11px;");
    pageRow->addWidget(pageLabel);
    pageEdit_ = new QLineEdit();
    pageEdit_->setReadOnly(true);
    pageEdit_->setMaximumWidth(80);
    pageRow->addWidget(pageEdit_);
    pageRow->addStretch();
    rightLayout->addLayout(pageRow);

    contentEdit_ = new QTextEdit();
    contentEdit_->setReadOnly(true);
    contentEdit_->setPlaceholderText("Select a note to view...");
    contentEdit_->setStyleSheet("font-size: 13px;");
    rightLayout->addWidget(contentEdit_, 1);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    // Close
    auto* closeRow = new QHBoxLayout();
    closeRow->addStretch();
    auto* closeBtn = new QPushButton("Close");
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    closeRow->addWidget(closeBtn);
    layout->addLayout(closeRow);
}

void PaperNotesDialog::loadNotes() {
    // Try loading from local storage or API
    // For now, show empty state
    refreshList();
}

void PaperNotesDialog::refreshList() {
    notesList_->clear();
    for (int i = 0; i < notes_.size(); ++i) {
        const auto& note = notes_[i];
        QString preview = note.content;
        if (preview.length() > 50) preview = preview.left(50) + "...";
        QString label = QString("#%1 — %2").arg(i + 1).arg(preview);
        if (!note.highlight.isEmpty()) {
            label += QString(" [%1]").arg(note.highlight.left(20));
        }
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, i);
        notesList_->addItem(item);
    }

    if (notes_.isEmpty()) {
        contentEdit_->setPlaceholderText("No notes yet. Click '+' to add one.");
    }
}

void PaperNotesDialog::onNoteSelected(int row) {
    selectedIdx_ = row;
    if (row < 0 || row >= notes_.size()) {
        contentEdit_->clear();
        highlightEdit_->clear();
        pageEdit_->clear();
        editBtn_->setEnabled(false);
        deleteBtn_->setEnabled(false);
        return;
    }

    const auto& note = notes_[row];
    contentEdit_->setPlainText(note.content);
    highlightEdit_->setText(note.highlight);
    pageEdit_->setText(note.page > 0 ? QString::number(note.page) : "");
    editBtn_->setEnabled(true);
    deleteBtn_->setEnabled(true);
}

void PaperNotesDialog::onAddNote() {
    QDialog dlg(this);
    dlg.setWindowTitle("Add Note");
    auto* form = new QFormLayout(&dlg);

    auto* contentEdit = new QTextEdit();
    contentEdit->setMinimumHeight(120);
    form->addRow("Note:", contentEdit);

    auto* highlightEdit = new QLineEdit();
    highlightEdit->setPlaceholderText("Optional: text from paper");
    form->addRow("Highlight:", highlightEdit);

    auto* pageEdit = new QLineEdit();
    pageEdit->setPlaceholderText("Optional");
    form->addRow("Page:", pageEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        PaperNote note;
        note.paperId = paperId_;
        note.content = contentEdit->toPlainText();
        note.highlight = highlightEdit->text();
        note.page = pageEdit->text().toInt();
        note.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);

        if (!note.content.trimmed().isEmpty()) {
            notes_.append(note);
            refreshList();
            notesList_->setCurrentRow(notes_.size() - 1);
        }
    }
}

void PaperNotesDialog::onEditNote() {
    if (selectedIdx_ < 0 || selectedIdx_ >= notes_.size()) return;

    auto& note = notes_[selectedIdx_];

    QDialog dlg(this);
    dlg.setWindowTitle("Edit Note");
    auto* form = new QFormLayout(&dlg);

    auto* contentEdit = new QTextEdit();
    contentEdit->setPlainText(note.content);
    contentEdit->setMinimumHeight(120);
    form->addRow("Note:", contentEdit);

    auto* highlightEdit = new QLineEdit();
    highlightEdit->setText(note.highlight);
    form->addRow("Highlight:", highlightEdit);

    auto* pageEdit = new QLineEdit();
    pageEdit->setText(note.page > 0 ? QString::number(note.page) : "");
    form->addRow("Page:", pageEdit);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    form->addRow(buttons);

    if (dlg.exec() == QDialog::Accepted) {
        note.content = contentEdit->toPlainText();
        note.highlight = highlightEdit->text();
        note.page = pageEdit->text().toInt();
        refreshList();
        notesList_->setCurrentRow(selectedIdx_);
    }
}

void PaperNotesDialog::onDeleteNote() {
    if (selectedIdx_ < 0 || selectedIdx_ >= notes_.size()) return;

    auto result = QMessageBox::question(this, "Delete Note", "Delete this note?");
    if (result == QMessageBox::Yes) {
        notes_.removeAt(selectedIdx_);
        selectedIdx_ = -1;
        refreshList();
        contentEdit_->clear();
        highlightEdit_->clear();
        pageEdit_->clear();
    }
}
