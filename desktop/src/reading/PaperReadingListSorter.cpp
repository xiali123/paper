#include "reading/PaperReadingListSorter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <algorithm>

PaperReadingListSorter::PaperReadingListSorter(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingListSorter")
{
    setupUI();
    loadSettings();
}

void PaperReadingListSorter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Paper");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingListSorter::onAdd);
    toolbar->addWidget(addBtn_);

    sortBtn_ = new QPushButton("Sort");
    sortBtn_->setStyleSheet("QPushButton { background: #16a34a; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(sortBtn_, &QPushButton::clicked, this, &PaperReadingListSorter::onSort);
    toolbar->addWidget(sortBtn_);

    toolbar->addWidget(new QLabel("Sort by:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Priority", "Relevance", "Pages", "Difficulty"});
    toolbar->addWidget(sortCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingListSorter::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Sort your reading list");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingListSorter::addEntry(const SortEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit listSorted(entries_.size());
    update();
}

QList<SortEntry> PaperReadingListSorter::entries() const { return entries_; }

QMap<QString, int> PaperReadingListSorter::difficultyCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.difficulty]++;
    return counts;
}

qreal PaperReadingListSorter::avgRelevance() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.relevance;
    return sum / entries_.size();
}

int PaperReadingListSorter::totalPages() const {
    int t = 0;
    for (const auto& e : entries_) t += e.pageCount;
    return t;
}

void PaperReadingListSorter::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Add Paper", "Title:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;

    QStringList diffs = {"easy", "medium", "hard"};
    QString diff = QInputDialog::getItem(this, "Add Paper", "Difficulty:", diffs, 1, false, &ok);
    if (!ok) return;

    SortEntry e;
    e.id = entries_.size() + 1;
    e.paperTitle = title;
    e.category = "Research";
    e.priority = 1 + QRandomGenerator::global()->bounded(5);
    e.relevance = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    e.pageCount = 5 + QRandomGenerator::global()->bounded(40);
    e.difficulty = diff;
    e.status = "pending";

    QColor diffColors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};
    int dIdx = diffs.indexOf(diff);
    e.color = diffColors[qBound(0, dIdx, 2)];
    addEntry(e);
}

void PaperReadingListSorter::onSort() {
    int sortIdx = sortCombo_->currentIndex();
    switch (sortIdx) {
    case 0: std::sort(entries_.begin(), entries_.end(), [](const SortEntry& a, const SortEntry& b) { return a.priority > b.priority; }); break;
    case 1: std::sort(entries_.begin(), entries_.end(), [](const SortEntry& a, const SortEntry& b) { return a.relevance > b.relevance; }); break;
    case 2: std::sort(entries_.begin(), entries_.end(), [](const SortEntry& a, const SortEntry& b) { return a.pageCount < b.pageCount; }); break;
    case 3: {
        QStringList order = {"easy", "medium", "hard"};
        std::sort(entries_.begin(), entries_.end(), [&order](const SortEntry& a, const SortEntry& b) {
            return order.indexOf(a.difficulty) < order.indexOf(b.difficulty);
        });
        break;
    }
    }
    saveSettings();
    updateInfo();
    emit listSorted(entries_.size());
    update();
}

void PaperReadingListSorter::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Sort your reading list");
    update();
}

void PaperReadingListSorter::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Sort your reading list");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading List Sorter");

    int w = width(), h = height();
    drawSortedList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawDifficultyChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingListSorter::drawSortedList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   "#" + QString::number(i + 1) + " " + e.paperTitle.left(16));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.difficulty + " | pri:" + QString::number(e.priority));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.relevance * 100, 'f', 0) + "% rel");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.pageCount) + " pages");
    }
}

void PaperReadingListSorter::drawDifficultyChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Difficulty");

    auto counts = difficultyCounts();
    QStringList diffs = {"easy", "medium", "hard"};
    QString labels[] = {"Easy", "Medium", "Hard"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(diffs[i]) ? counts[diffs[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 70, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperReadingListSorter::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(entries_.size()), QColor(59,130,246)},
        {"Total Pages", QString::number(totalPages()), QColor(16,185,129)},
        {"Avg Relevance", QString::number(avgRelevance() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Hard", QString::number(difficultyCounts().value("hard", 0)), QColor(239,68,68)}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperReadingListSorter::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Sort your reading list"); return; }
    infoLabel_->setText(QString("%1 papers | %2 pages | %3% rel avg")
        .arg(entries_.size()).arg(totalPages()).arg(avgRelevance() * 100, 0, 'f', 0));
}

void PaperReadingListSorter::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SortEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.category = settings_.value("category").toString();
        e.priority = settings_.value("priority").toInt();
        e.relevance = settings_.value("relevance").toDouble();
        e.pageCount = settings_.value("pageCount").toInt();
        e.difficulty = settings_.value("difficulty").toString();
        e.status = settings_.value("status").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingListSorter::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("priority", entries_[i].priority);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("pageCount", entries_[i].pageCount);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
