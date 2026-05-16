#include "workspace/PaperCommentWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PaperCommentWidget::PaperCommentWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperCommentWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Unresolved", "Resolved", "Questions", "Notes", "Critique"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCommentWidget::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    tree_ = new QTreeWidget();
    tree_->setHeaderLabels({"Comment", "Author", "Time", "Status"});
    tree_->setColumnWidth(0, 300);
    tree_->setColumnWidth(1, 80);
    tree_->setColumnWidth(2, 120);
    tree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(tree_, &QTreeWidget::itemClicked, this, &PaperCommentWidget::onItemClicked);
    layout->addWidget(tree_, 1);

    auto* inputRow = new QHBoxLayout();
    authorEdit_ = new QLineEdit("Me");
    authorEdit_->setMaximumWidth(100);
    authorEdit_->setPlaceholderText("Author");
    inputRow->addWidget(authorEdit_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Note", "Question", "Critique", "Idea", "Summary"});
    inputRow->addWidget(categoryCombo_);
    layout->addLayout(inputRow);

    inputEdit_ = new QTextEdit();
    inputEdit_->setMaximumHeight(80);
    inputEdit_->setPlaceholderText("Write a comment...");
    layout->addWidget(inputEdit_);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Comment");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperCommentWidget::onAdd);
    btnRow->addWidget(addBtn_);

    replyBtn_ = new QPushButton("Reply");
    connect(replyBtn_, &QPushButton::clicked, this, &PaperCommentWidget::onReply);
    btnRow->addWidget(replyBtn_);

    resolveBtn_ = new QPushButton("Resolve");
    resolveBtn_->setStyleSheet("color: #059669;");
    connect(resolveBtn_, &QPushButton::clicked, this, &PaperCommentWidget::onResolve);
    btnRow->addWidget(resolveBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperCommentWidget::onDelete);
    btnRow->addWidget(deleteBtn_);

    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 comments");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperCommentWidget::setPaper(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Comments: %1").arg(title));
    refreshTree();
    updateStats();
}

void PaperCommentWidget::addComment(const Comment& comment) {
    Comment c = comment;
    if (c.id < 0) c.id = nextId_++;
    if (c.timestamp == 0) c.timestamp = QDateTime::currentSecsSinceEpoch();
    allComments_.append(c);
    nextId_ = qMax(nextId_, c.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
    emit commentAdded(c.paperId, c.id);
}

void PaperCommentWidget::addReply(int parentId, const QString& text, const QString& author) {
    Comment c;
    c.paperId = currentPaperId_;
    c.parentId = parentId;
    c.author = author;
    c.text = text;
    c.category = "Reply";
    addComment(c);
}

void PaperCommentWidget::resolveComment(int commentId) {
    for (auto& c : allComments_) {
        if (c.id == commentId) {
            c.resolved = true;
            refreshTree();
            saveSettings();
            emit commentResolved(c.paperId, commentId);
            break;
        }
    }
}

void PaperCommentWidget::deleteComment(int commentId) {
    allComments_.removeIf([commentId](const Comment& c) { return c.id == commentId; });
    refreshTree();
    saveSettings();
    updateStats();
    emit commentDeleted(commentId);
}

QList<Comment> PaperCommentWidget::comments(int paperId) const {
    QList<Comment> result;
    for (const auto& c : allComments_) if (c.paperId == paperId) result.append(c);
    return result;
}

int PaperCommentWidget::unresolvedCount(int paperId) const {
    int count = 0;
    for (const auto& c : allComments_)
        if (c.paperId == paperId && !c.resolved) count++;
    return count;
}

void PaperCommentWidget::onAdd() {
    if (currentPaperId_ < 0 || inputEdit_->toPlainText().trimmed().isEmpty()) return;
    Comment c;
    c.paperId = currentPaperId_;
    c.author = authorEdit_->text();
    c.text = inputEdit_->toPlainText();
    c.category = categoryCombo_->currentText();
    addComment(c);
    inputEdit_->clear();
}

void PaperCommentWidget::onReply() {
    auto* item = tree_->currentItem();
    if (!item) return;
    int parentId = item->data(0, Qt::UserRole).toInt();
    if (inputEdit_->toPlainText().trimmed().isEmpty()) return;
    addReply(parentId, inputEdit_->toPlainText(), authorEdit_->text());
    inputEdit_->clear();
}

void PaperCommentWidget::onResolve() {
    auto* item = tree_->currentItem();
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();
    resolveComment(id);
}

void PaperCommentWidget::onDelete() {
    auto* item = tree_->currentItem();
    if (!item) return;
    int id = item->data(0, Qt::UserRole).toInt();
    deleteComment(id);
}

void PaperCommentWidget::onFilterChanged(int) { refreshTree(); }

void PaperCommentWidget::onItemClicked(QTreeWidgetItem* item, int) {
    if (!item) return;
}

void PaperCommentWidget::refreshTree() {
    tree_->clear();
    if (currentPaperId_ < 0) return;

    int filter = filterCombo_->currentIndex();
    QList<Comment> paperComments;
    for (const auto& c : allComments_) {
        if (c.paperId != currentPaperId_) continue;
        if (filter == 1 && c.resolved) continue;
        if (filter == 2 && !c.resolved) continue;
        if (filter == 3 && c.category != "Question") continue;
        if (filter == 4 && c.category != "Note") continue;
        if (filter == 5 && c.category != "Critique") continue;
        paperComments.append(c);
    }

    // Build tree for top-level comments
    for (const auto& c : paperComments) {
        if (c.parentId >= 0) continue;
        QString text = QString("[%1] %2").arg(c.category, c.text.left(80));
        auto* item = new QTreeWidgetItem({text, c.author,
            QDateTime::fromSecsSinceEpoch(c.timestamp).toString("MM-dd HH:mm"),
            c.resolved ? "Resolved" : "Open"});
        item->setData(0, Qt::UserRole, c.id);
        if (c.resolved) item->setForeground(3, QColor(5, 150, 105));
        else item->setForeground(3, QColor(245, 158, 11));

        // Replies
        for (const auto& r : paperComments) {
            if (r.parentId != c.id) continue;
            auto* replyItem = new QTreeWidgetItem(item);
            replyItem->setText(0, "↳ " + r.text.left(60));
            replyItem->setText(1, r.author);
            replyItem->setText(2, QDateTime::fromSecsSinceEpoch(r.timestamp).toString("MM-dd HH:mm"));
            replyItem->setData(0, Qt::UserRole, r.id);
        }
        tree_->addTopLevelItem(item);
    }
    tree_->expandAll();
}

void PaperCommentWidget::updateStats() {
    if (currentPaperId_ < 0) { statsLabel_->setText("0 comments"); return; }
    int total = 0, unresolved = 0;
    for (const auto& c : allComments_) {
        if (c.paperId == currentPaperId_) { total++; if (!c.resolved) unresolved++; }
    }
    statsLabel_->setText(QString("%1 comments (%2 unresolved)").arg(total).arg(unresolved));
}

void PaperCommentWidget::loadSettings() {
    QSettings settings("PaperCrawler", "PaperComments");
    QByteArray data = settings.value("comments").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        Comment c;
        c.id = obj["id"].toInt();
        c.paperId = obj["paperId"].toInt();
        c.parentId = obj["parentId"].toInt();
        c.author = obj["author"].toString();
        c.text = obj["text"].toString();
        c.category = obj["category"].toString();
        c.timestamp = obj["timestamp"].toInteger();
        c.resolved = obj["resolved"].toBool();
        allComments_.append(c);
        nextId_ = qMax(nextId_, c.id + 1);
    }
}

void PaperCommentWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& c : allComments_) {
        QJsonObject obj;
        obj["id"] = c.id;
        obj["paperId"] = c.paperId;
        obj["parentId"] = c.parentId;
        obj["author"] = c.author;
        obj["text"] = c.text;
        obj["category"] = c.category;
        obj["timestamp"] = static_cast<qint64>(c.timestamp);
        obj["resolved"] = c.resolved;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "PaperComments");
    settings.setValue("comments", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
