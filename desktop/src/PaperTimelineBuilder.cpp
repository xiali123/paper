#include "PaperTimelineBuilder.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QTextStream>

PaperTimelineBuilder::PaperTimelineBuilder(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperTimelineBuilder::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Group by:"));
    groupCombo_ = new QComboBox();
    groupCombo_->addItems({"Year", "Category", "Month", "None"});
    connect(groupCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTimelineBuilder::onGroupChanged);
    toolbar->addWidget(groupCombo_, 1);

    exportBtn_ = new QPushButton("Export Timeline");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperTimelineBuilder::onExport);
    toolbar->addWidget(exportBtn_);

    toolbar->addStretch();
    layout->addLayout(toolbar);

    entryList_ = new QListWidget();
    entryList_->setStyleSheet(
        "QListWidget { border: none; background: transparent; }"
        "QListWidget::item { padding: 8px; margin: 4px 0; border-left: 3px solid #3b82f6; "
        "background: #f8fafc; border-radius: 0 6px 6px 0; }"
        "QListWidget::item:selected { background: #dbeafe; border-left-color: #2563eb; }"
        "QListWidget::item:hover { background: #f1f5f9; }"
    );
    connect(entryList_, &QListWidget::itemClicked, this, &PaperTimelineBuilder::onItemClicked);
    layout->addWidget(entryList_, 1);

    infoLabel_ = new QLabel("Add papers to build timeline");
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(infoLabel_);
}

void PaperTimelineBuilder::addEntry(const TimelineEntry& entry) {
    entries_.append(entry);
    rebuildTimeline();
}

void PaperTimelineBuilder::addEntries(const QList<TimelineEntry>& entries) {
    for (const auto& e : entries) entries_.append(e);
    rebuildTimeline();
}

void PaperTimelineBuilder::clearTimeline() {
    entries_.clear();
    entryList_->clear();
    infoLabel_->setText("Timeline cleared");
}

QList<TimelineEntry> PaperTimelineBuilder::entries() const { return entries_; }

void PaperTimelineBuilder::setGroupBy(const QString& field) {
    groupField_ = field;
    rebuildTimeline();
}

void PaperTimelineBuilder::onGroupChanged(int index) {
    QStringList fields = {"year", "category", "month", "none"};
    if (index >= 0 && index < fields.size()) groupField_ = fields[index];
    rebuildTimeline();
}

void PaperTimelineBuilder::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Timeline", "timeline.txt", "Text (*.txt)");
    if (path.isEmpty()) return;
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);

    QMap<int, QList<TimelineEntry>> byYear;
    for (const auto& e : entries_) byYear[e.year].append(e);

    for (auto it = byYear.begin(); it != byYear.end(); ++it) {
        out << QString("=== %1 ===\n").arg(it.key());
        for (const auto& e : it.value()) {
            out << QString("  [%1] %2\n").arg(e.category, e.title);
            if (!e.description.isEmpty())
                out << QString("      %1\n").arg(e.description);
        }
        out << "\n";
    }
    emit timelineExported();
}

void PaperTimelineBuilder::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int id = item->data(Qt::UserRole).toInt();
    emit entryClicked(id);
}

void PaperTimelineBuilder::refreshList() {
    entryList_->clear();
    for (const auto& e : entries_) {
        QString label = QString("[%1] %2").arg(e.category, e.title);
        if (e.year > 0) label = QString("%1 — %2").arg(e.year).arg(label);
        auto* item = new QListWidgetItem(label);
        item->setData(Qt::UserRole, e.paperId);
        item->setForeground(e.color);
        entryList_->addItem(item);
    }
}

void PaperTimelineBuilder::rebuildTimeline() {
    if (entries_.isEmpty()) {
        entryList_->clear();
        infoLabel_->setText("No entries");
        return;
    }

    // Sort by date
    QList<TimelineEntry> sorted = entries_;
    std::sort(sorted.begin(), sorted.end(),
        [](const TimelineEntry& a, const TimelineEntry& b) {
            if (a.year != b.year) return a.year < b.year;
            return a.month < b.month;
        });

    entryList_->clear();

    if (groupField_ == "none") {
        for (const auto& e : sorted) {
            QString label = QString("%1 %2 — [%3] %4")
                .arg(e.year).arg(e.month > 0 ? QString("M%1").arg(e.month) : "")
                .arg(e.category, e.title);
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, e.paperId);
            entryList_->addItem(item);
        }
    } else if (groupField_ == "year") {
        int lastYear = -1;
        for (const auto& e : sorted) {
            if (e.year != lastYear) {
                auto* header = new QListWidgetItem(QString("── %1 ──").arg(e.year));
                header->setFlags(header->flags() & ~Qt::ItemIsSelectable);
                QFont font = header->font();
                font.setBold(true);
                font.setPointSize(font.pointSize() + 1);
                header->setFont(font);
                header->setForeground(QColor(30, 41, 59));
                header->setBackground(QColor(241, 245, 249));
                entryList_->addItem(header);
                lastYear = e.year;
            }
            QString label = QString("    %1 [%2] %3")
                .arg(e.month > 0 ? QString("M%1").arg(e.month, 2, 10, QChar('0')) : "    ")
                .arg(e.category, e.title);
            auto* item = new QListWidgetItem(label);
            item->setData(Qt::UserRole, e.paperId);
            item->setForeground(e.color);
            entryList_->addItem(item);
        }
    } else if (groupField_ == "category") {
        QMap<QString, QList<TimelineEntry>> grouped;
        for (const auto& e : sorted) grouped[e.category].append(e);
        for (auto it = grouped.begin(); it != grouped.end(); ++it) {
            auto* header = new QListWidgetItem(QString("── %1 (%2) ──").arg(it.key()).arg(it.value().size()));
            header->setFlags(header->flags() & ~Qt::ItemIsSelectable);
            QFont font = header->font();
            font.setBold(true);
            header->setFont(font);
            header->setForeground(QColor(139, 92, 246));
            entryList_->addItem(header);

            for (const auto& e : it.value()) {
                QString label = QString("    %1 — %2").arg(e.year).arg(e.title);
                auto* item = new QListWidgetItem(label);
                item->setData(Qt::UserRole, e.paperId);
                entryList_->addItem(item);
            }
        }
    } else if (groupField_ == "month") {
        QMap<int, QList<TimelineEntry>> byMonth;
        for (const auto& e : sorted) {
            int key = e.year * 100 + qMax(1, e.month);
            byMonth[key].append(e);
        }
        for (auto it = byMonth.begin(); it != byMonth.end(); ++it) {
            int year = it.key() / 100;
            int month = it.key() % 100;
            auto* header = new QListWidgetItem(QString("── %1/%2 ──").arg(year).arg(month, 2, 10, QChar('0')));
            header->setFlags(header->flags() & ~Qt::ItemIsSelectable);
            QFont font = header->font();
            font.setBold(true);
            header->setFont(font);
            entryList_->addItem(header);

            for (const auto& e : it.value()) {
                auto* item = new QListWidgetItem("    " + e.title);
                item->setData(Qt::UserRole, e.paperId);
                entryList_->addItem(item);
            }
        }
    }

    infoLabel_->setText(QString("%1 entries, grouped by %2")
        .arg(entries_.size()).arg(groupField_));
}
