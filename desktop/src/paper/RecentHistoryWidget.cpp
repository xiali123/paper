#include "paper/RecentHistoryWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QStandardPaths>
#include <QDir>

HistoryEntry HistoryEntry::fromJson(const QJsonObject& json) {
    HistoryEntry e;
    e.paperId = json["paperId"].toInt();
    e.title = json["title"].toString();
    e.authors = json["authors"].toString();
    e.year = json["year"].toString();
    e.viewedAt = QDateTime::fromString(json["viewedAt"].toString(), Qt::ISODate);
    return e;
}

QJsonObject HistoryEntry::toJson() const {
    QJsonObject j;
    j["paperId"] = paperId;
    j["title"] = title;
    j["authors"] = authors;
    j["year"] = year;
    j["viewedAt"] = viewedAt.toString(Qt::ISODate);
    return j;
}

RecentHistoryWidget::RecentHistoryWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadHistory();
}

void RecentHistoryWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);

    auto* headerRow = new QHBoxLayout();
    auto* header = new QLabel("Recently Viewed");
    header->setStyleSheet("font-weight: bold; font-size: 12px;");
    headerRow->addWidget(header);

    countLabel_ = new QLabel("0");
    countLabel_->setStyleSheet("color: palette(mid); font-size: 11px;");
    headerRow->addWidget(countLabel_);
    headerRow->addStretch();

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setFixedWidth(50);
    clearBtn_->setStyleSheet("font-size: 11px;");
    connect(clearBtn_, &QPushButton::clicked, this, &RecentHistoryWidget::onClear);
    headerRow->addWidget(clearBtn_);
    layout->addLayout(headerRow);

    historyList_ = new QListWidget();
    historyList_->setStyleSheet(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item { padding: 4px 8px; border-bottom: 1px solid palette(mid); }"
        "QListWidget::item:hover { background: palette(alternate-base); }"
    );
    connect(historyList_, &QListWidget::itemClicked, this, &RecentHistoryWidget::onItemClicked);
    layout->addWidget(historyList_, 1);

    setMaximumHeight(300);
    setMaximumWidth(300);
}

void RecentHistoryWidget::addEntry(int paperId, const QString& title,
                                    const QString& authors, const QString& year) {
    // Remove duplicate
    for (int i = 0; i < entries_.size(); ++i) {
        if (entries_[i].paperId == paperId) {
            entries_.removeAt(i);
            break;
        }
    }

    HistoryEntry e;
    e.paperId = paperId;
    e.title = title;
    e.authors = authors;
    e.year = year;
    e.viewedAt = QDateTime::currentDateTime();

    entries_.prepend(e);

    // Trim to max
    while (entries_.size() > MAX_ENTRIES) {
        entries_.removeLast();
    }

    saveHistory();
    refreshList();
}

void RecentHistoryWidget::loadHistory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);
    QFile file(path + "/view_history.json");

    if (file.open(QIODevice::ReadOnly)) {
        QJsonArray arr = QJsonDocument::fromJson(file.readAll()).array();
        for (const auto& v : arr) {
            entries_.append(HistoryEntry::fromJson(v.toObject()));
        }
    }

    refreshList();
}

void RecentHistoryWidget::saveHistory() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(path);

    QJsonArray arr;
    for (const auto& e : entries_) {
        arr.append(e.toJson());
    }

    QFile file(path + "/view_history.json");
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }
}

void RecentHistoryWidget::clearHistory() {
    entries_.clear();
    saveHistory();
    refreshList();
}

QList<int> RecentHistoryWidget::recentPaperIds(int limit) const {
    QList<int> ids;
    for (int i = 0; i < qMin(entries_.size(), limit); ++i) {
        ids.append(entries_[i].paperId);
    }
    return ids;
}

void RecentHistoryWidget::refreshList() {
    historyList_->clear();
    for (const auto& e : entries_) {
        QString label = QString("%1 (%2)").arg(e.title.left(40), e.year);
        if (!e.authors.isEmpty()) {
            label += QString("\n%1").arg(e.authors.left(30));
        }
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, e.paperId);
        item->setToolTip(QString("%1\n%2\nViewed: %3")
            .arg(e.title, e.authors, e.viewedAt.toString("yyyy-MM-dd HH:mm")));
        historyList_->addItem(item);
    }
    countLabel_->setText(QString::number(entries_.size()));
}

void RecentHistoryWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int paperId = item->data(Qt::UserRole).toInt();
    if (paperId > 0) emit openPaperRequested(paperId);
}

void RecentHistoryWidget::onClear() {
    clearHistory();
}
