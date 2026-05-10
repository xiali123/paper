#include "tools/PaperRegexSearchTool.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperRegexSearchTool::PaperRegexSearchTool(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "RegexSearchTool")
{
    setupUI();
    loadSettings();
}

void PaperRegexSearchTool::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    searchBtn_ = new QPushButton("Search");
    searchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(searchBtn_, &QPushButton::clicked, this, &PaperRegexSearchTool::onSearch);
    toolbar->addWidget(searchBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Authors", "Dates", "DOIs", "Emails"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperRegexSearchTool::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    patternField_ = new QLineEdit();
    patternField_->setPlaceholderText("Enter regex pattern (e.g., \\b\\d{4}\\b, \\w+@\\w+\\.edu)...");
    patternField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; font-family: monospace; }");
    layout->addWidget(patternField_);

    infoLabel_ = new QLabel("Search with regex patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperRegexSearchTool::addEntry(const RegexEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit searchComplete(entry.id, entry.matchCount);
    update();
}

QList<RegexEntry> PaperRegexSearchTool::entries() const { return entries_; }

int PaperRegexSearchTool::totalMatches() const {
    int t = 0;
    for (const auto& e : entries_) t += e.matchCount;
    return t;
}

qreal PaperRegexSearchTool::avgMatchTime() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.matchTime;
    return sum / entries_.size();
}

QMap<QString, int> PaperRegexSearchTool::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperRegexSearchTool::onSearch() {
    QString pattern = patternField_->text().trimmed();
    if (pattern.isEmpty()) return;

    QStringList categories = {"authors", "dates", "dois", "emails", "custom"};
    QStringList sampleMatches = {"Smith et al.", "2024-01-15", "10.1234/abc", "user@edu", pattern.left(10)};
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int catIdx = categoryCombo_->currentIndex();
    if (catIdx == 0) catIdx = 4;

    RegexEntry e;
    e.id = entries_.size() + 1;
    e.pattern = pattern;
    e.matchCount = QRandomGenerator::global()->bounded(50);
    e.source = "Paper collection";
    e.flags = "gi";
    e.category = categories[qMin(catIdx, categories.size() - 1)];
    e.matchTime = 1 + QRandomGenerator::global()->bounded(100);
    e.sampleMatch = sampleMatches[qMin(catIdx, sampleMatches.size() - 1)];
    e.color = catColors[qMin(catIdx, 4)];
    addEntry(e);
    patternField_->clear();
}

void PaperRegexSearchTool::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Search with regex patterns");
    update();
}

void PaperRegexSearchTool::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Search with regex patterns");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Regex Search Tool");

    int w = width(), h = height();
    drawResultList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperRegexSearchTool::drawResultList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Courier", 7, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.pattern.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | /" + e.pattern.left(8) + "/" + e.flags);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.matchCount) + " matches");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.matchTime) + "ms | " + e.sampleMatch.left(12));
    }
}

void PaperRegexSearchTool::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"authors", "dates", "dois", "emails", "custom"};
    QString labels[] = {"Authors", "Dates", "DOIs", "Emails", "Custom"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(18, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 2);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperRegexSearchTool::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Searches", QString::number(entries_.size()), QColor(59,130,246)},
        {"Matches", QString::number(totalMatches()), QColor(16,185,129)},
        {"Avg Time", QString::number(avgMatchTime(), 'f', 0) + "ms", QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperRegexSearchTool::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Search with regex patterns"); return; }
    infoLabel_->setText(QString("%1 searches | %2 matches | %3ms avg")
        .arg(entries_.size()).arg(totalMatches()).arg(avgMatchTime(), 0, 'f', 0));
}

void PaperRegexSearchTool::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RegexEntry e;
        e.id = settings_.value("id").toInt();
        e.pattern = settings_.value("pattern").toString();
        e.matchCount = settings_.value("matchCount").toInt();
        e.source = settings_.value("source").toString();
        e.flags = settings_.value("flags").toString();
        e.category = settings_.value("category").toString();
        e.matchTime = settings_.value("matchTime").toDouble();
        e.sampleMatch = settings_.value("sampleMatch").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperRegexSearchTool::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("pattern", entries_[i].pattern);
        settings_.setValue("matchCount", entries_[i].matchCount);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("flags", entries_[i].flags);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("matchTime", entries_[i].matchTime);
        settings_.setValue("sampleMatch", entries_[i].sampleMatch);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
