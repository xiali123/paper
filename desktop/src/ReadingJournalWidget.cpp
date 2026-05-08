#include "ReadingJournalWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDateTime>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>

ReadingJournalWidget::ReadingJournalWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void ReadingJournalWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Search + filter
    auto* topRow = new QHBoxLayout();
    searchEdit_ = new QLineEdit();
    searchEdit_->setPlaceholderText("Search journal...");
    topRow->addWidget(searchEdit_, 1);

    searchBtn_ = new QPushButton("Search");
    connect(searchBtn_, &QPushButton::clicked, this, &ReadingJournalWidget::onSearch);
    topRow->addWidget(searchBtn_);

    topRow->addWidget(new QLabel("Mood:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Productive", "Focused", "Neutral", "Tired", "Excited", "Confused"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ReadingJournalWidget::onFilterChanged);
    topRow->addWidget(filterCombo_);
    layout->addLayout(topRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Entry tree
    entryTree_ = new QTreeWidget();
    entryTree_->setHeaderLabels({"Date", "Title", "Mood", "Tags"});
    entryTree_->setColumnWidth(0, 80);
    entryTree_->setColumnWidth(1, 180);
    entryTree_->setColumnWidth(2, 70);
    entryTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(entryTree_, &QTreeWidget::itemClicked, this, &ReadingJournalWidget::onEntrySelected);
    splitter->addWidget(entryTree_);

    // Editor
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* formRow = new QHBoxLayout();
    formRow->addWidget(new QLabel("Date:"));
    dateEdit_ = new QDateEdit(QDate::currentDate());
    dateEdit_->setCalendarPopup(true);
    dateEdit_->setDisplayFormat("yyyy-MM-dd");
    formRow->addWidget(dateEdit_);

    todayBtn_ = new QPushButton("Today");
    connect(todayBtn_, &QPushButton::clicked, this, &ReadingJournalWidget::onToday);
    formRow->addWidget(todayBtn_);

    formRow->addWidget(new QLabel("Title:"));
    titleEdit_ = new QLineEdit();
    titleEdit_->setPlaceholderText("Entry title...");
    formRow->addWidget(titleEdit_, 1);

    formRow->addWidget(new QLabel("Mood:"));
    moodCombo_ = new QComboBox();
    moodCombo_->addItems({"Neutral", "Productive", "Focused", "Tired", "Excited", "Confused"});
    formRow->addWidget(moodCombo_);
    rightLayout->addLayout(formRow);

    contentEdit_ = new QTextEdit();
    contentEdit_->setPlaceholderText("Write your reading journal entry...");
    contentEdit_->setStyleSheet("QTextEdit { border: 1px solid palette(mid); border-radius: 4px; }");
    rightLayout->addWidget(contentEdit_, 1);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Save Entry");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &ReadingJournalWidget::onAdd);
    btnRow->addWidget(addBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &ReadingJournalWidget::onDelete);
    btnRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &ReadingJournalWidget::onExport);
    btnRow->addWidget(exportBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 entries");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void ReadingJournalWidget::addEntry(const JournalEntry& entry) {
    JournalEntry e = entry;
    if (e.id < 0) e.id = nextId_++;
    if (e.createdAt == 0) e.createdAt = QDateTime::currentSecsSinceEpoch();
    entries_.append(e);
    nextId_ = qMax(nextId_, e.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
    emit entryAdded(e.id);
}

void ReadingJournalWidget::removeEntry(int id) {
    entries_.removeIf([id](const JournalEntry& e) { return e.id == id; });
    if (selectedId_ == id) selectedId_ = -1;
    refreshTree();
    saveSettings();
    updateStats();
    emit entryRemoved(id);
}

QList<JournalEntry> ReadingJournalWidget::entries() const { return entries_; }

QList<JournalEntry> ReadingJournalWidget::entriesByDate(const QDate& from, const QDate& to) const {
    QList<JournalEntry> result;
    for (const auto& e : entries_) {
        if (e.date >= from && e.date <= to) result.append(e);
    }
    return result;
}

QList<JournalEntry> ReadingJournalWidget::searchEntries(const QString& query) const {
    QList<JournalEntry> result;
    QString q = query.toLower();
    for (const auto& e : entries_) {
        if (e.title.toLower().contains(q) || e.content.toLower().contains(q)) result.append(e);
    }
    return result;
}

void ReadingJournalWidget::onAdd() {
    if (titleEdit_->text().trimmed().isEmpty() && contentEdit_->toPlainText().trimmed().isEmpty()) return;
    JournalEntry e;
    e.date = dateEdit_->date();
    e.title = titleEdit_->text().trimmed().isEmpty() ? "Untitled" : titleEdit_->text().trimmed();
    e.mood = moodCombo_->currentText().toLower();
    e.content = contentEdit_->toPlainText();
    addEntry(e);
    titleEdit_->clear();
    contentEdit_->clear();
}

void ReadingJournalWidget::onDelete() {
    if (selectedId_ < 0) return;
    removeEntry(selectedId_);
}

void ReadingJournalWidget::onSearch() {
    QString query = searchEdit_->text().trimmed();
    if (query.isEmpty()) { refreshTree(); return; }

    entryTree_->clear();
    auto results = searchEntries(query);
    for (const auto& e : results) {
        auto* item = new QTreeWidgetItem({
            e.date.toString("MM-dd"),
            e.title.left(30),
            e.mood,
            e.tags.join(", ")
        });
        item->setData(0, Qt::UserRole, e.id);
        entryTree_->addTopLevelItem(item);
    }
}

void ReadingJournalWidget::onFilterChanged(int) { refreshTree(); }

void ReadingJournalWidget::onEntrySelected() {
    auto* item = entryTree_->currentItem();
    if (!item) return;
    selectedId_ = item->data(0, Qt::UserRole).toInt();
    for (const auto& e : entries_) {
        if (e.id == selectedId_) {
            titleEdit_->setText(e.title);
            contentEdit_->setPlainText(e.content);
            dateEdit_->setDate(e.date);
            int mi = moodCombo_->findText(e.mood, Qt::MatchFixedString);
            if (mi < 0) mi = 0;
            moodCombo_->setCurrentIndex(mi);
            emit entrySelected(e.id, e.title);
            break;
        }
    }
}

void ReadingJournalWidget::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Journal", "", "JSON (*.json)");
    if (fileName.isEmpty()) return;
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["date"] = e.date.toString(Qt::ISODate);
        obj["title"] = e.title;
        obj["mood"] = e.mood;
        obj["content"] = e.content;
        obj["createdAt"] = static_cast<qint64>(e.createdAt);
        arr.append(obj);
    }
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(arr).toJson());
}

void ReadingJournalWidget::onToday() {
    dateEdit_->setDate(QDate::currentDate());
}

void ReadingJournalWidget::refreshTree() {
    entryTree_->clear();
    QString moodFilter = filterCombo_->currentText().toLower();
    QMap<QString, QColor> moodColors = {
        {"productive", QColor(16,185,129)}, {"focused", QColor(59,130,246)},
        {"neutral", QColor(148,163,184)}, {"tired", QColor(245,158,11)},
        {"excited", QColor(236,72,153)}, {"confused", QColor(239,68,68)}
    };

    for (const auto& e : entries_) {
        if (filterCombo_->currentIndex() > 0 && e.mood != moodFilter) continue;
        auto* item = new QTreeWidgetItem({
            e.date.toString("MM-dd"),
            e.title.left(30),
            e.mood,
            e.tags.join(", ")
        });
        item->setData(0, Qt::UserRole, e.id);
        if (moodColors.contains(e.mood)) item->setForeground(2, moodColors[e.mood]);
        entryTree_->addTopLevelItem(item);
    }
}

void ReadingJournalWidget::updateStats() {
    int total = entries_.size();
    int thisWeek = 0;
    QDate weekStart = QDate::currentDate().addDays(-7);
    for (const auto& e : entries_) {
        if (e.date >= weekStart) thisWeek++;
    }
    statsLabel_->setText(QString("%1 entries (%2 this week)").arg(total).arg(thisWeek));
}

void ReadingJournalWidget::loadSettings() {
    QSettings settings("PaperCrawler", "ReadingJournal");
    QByteArray data = settings.value("entries").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        JournalEntry e;
        e.id = obj["id"].toInt();
        e.date = QDate::fromString(obj["date"].toString(), Qt::ISODate);
        e.title = obj["title"].toString();
        e.mood = obj["mood"].toString();
        e.content = obj["content"].toString();
        e.createdAt = obj["createdAt"].toInteger();
        entries_.append(e);
        nextId_ = qMax(nextId_, e.id + 1);
    }
    refreshTree();
    updateStats();
}

void ReadingJournalWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& e : entries_) {
        QJsonObject obj;
        obj["id"] = e.id;
        obj["date"] = e.date.toString(Qt::ISODate);
        obj["title"] = e.title;
        obj["mood"] = e.mood;
        obj["content"] = e.content;
        obj["createdAt"] = static_cast<qint64>(e.createdAt);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "ReadingJournal");
    settings.setValue("entries", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
