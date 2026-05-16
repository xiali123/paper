#include "reading/PaperReadingDigest.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingDigest::PaperReadingDigest(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingDigest")
{
    setupUI();
    loadSettings();
}

void PaperReadingDigest::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(createBtn_, &QPushButton::clicked, this, &PaperReadingDigest::onCreate);
    toolbar->addWidget(createBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Survey", "Experimental", "Theoretical", "Applied", "Review"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingDigest::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper name for digest...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Create reading digests");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingDigest::addEntry(const DigestEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit digestCreated(entry.id, entry.quality);
    update();
}

QList<DigestEntry> PaperReadingDigest::entries() const { return entries_; }

int PaperReadingDigest::starredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.starred) c++;
    return c;
}

qreal PaperReadingDigest::avgQuality() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.quality;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingDigest::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingDigest::onCreate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"survey", "experimental", "theoretical", "applied", "review"};
    QStringList summaries = {
        "Comprehensive analysis of recent advances",
        "Novel methodology with strong empirical results",
        "Theoretical framework extending prior work",
        "Practical application demonstrating real-world impact",
        "Systematic review identifying key research gaps",
        "Benchmark study comparing state-of-the-art methods",
        "Interdisciplinary approach combining multiple domains",
        "Replication study confirming earlier findings"
    };
    QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };
    int cIdx = categoryCombo_->currentIndex();
    int count = 2 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        DigestEntry e;
        e.id = entries_.size() + 1;
        e.paper = text + " #" + QString::number(e.id);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.summary = summaries[QRandomGenerator::global()->bounded(summaries.size())];
        e.relevance = QRandomGenerator::global()->bounded(100) / 100.0;
        e.quality = 30 + QRandomGenerator::global()->bounded(71) / 100.0;
        e.date = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(28));
        e.starred = e.quality > 0.9;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingDigest::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Create reading digests");
    update();
}

void PaperReadingDigest::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Create reading digests");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Digest");
    int w = width(), h = height();
    drawDigestList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingDigest::drawDigestList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        // Background bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        // Left accent stripe
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        // Paper name + star indicator
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.paper.left(18) + (e.starred ? " *" : ""));
        // Summary line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.summary.left(30) + " | " + e.date);
        // Quality and relevance
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "Q:" + QString::number(e.quality, 'f', 2) + " R:" + QString::number(e.relevance, 'f', 2));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
    }
}

void PaperReadingDigest::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"survey", "experimental", "theoretical", "applied", "review"};
    QString labels[] = {"Survey", "Exper.", "Theory", "Applied", "Review"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 90));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 40, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 45, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 48 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingDigest::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Digests",   QString::number(entries_.size()), QColor(59, 130, 246)},
        {"Starred",   QString::number(starredCount()),  QColor(22, 163, 74)},
        {"Avg Qual",  QString::number(avgQuality(), 'f', 2), QColor(217, 119, 6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124, 58, 237)}
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

void PaperReadingDigest::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Create reading digests"); return; }
    infoLabel_->setText(QString("%1 digests | %2 starred | %3 avg quality")
        .arg(entries_.size()).arg(starredCount()).arg(avgQuality(), 0, 'f', 2));
}

void PaperReadingDigest::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DigestEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.summary = settings_.value("summary").toString();
        e.relevance = settings_.value("relevance").toDouble();
        e.quality = settings_.value("quality").toDouble();
        e.date = settings_.value("date").toString();
        e.starred = settings_.value("starred").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingDigest::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("summary", entries_[i].summary);
        settings_.setValue("relevance", entries_[i].relevance);
        settings_.setValue("quality", entries_[i].quality);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("starred", entries_[i].starred);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
