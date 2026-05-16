#include "reading/PaperReadingArchipelago.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingArchipelago::PaperReadingArchipelago(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingArchipelago")
{
    setupUI();
    loadSettings();
}

void PaperReadingArchipelago::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    exploreBtn_ = new QPushButton("Explore");
    exploreBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(exploreBtn_, &QPushButton::clicked, this, &PaperReadingArchipelago::onExplore);
    toolbar->addWidget(exploreBtn_);
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Theory", "Applied", "Survey", "Experimental"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter island name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingArchipelago::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Explore reading archipelago");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingArchipelago::addEntry(const ArchipelagoEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit islandExplored(entry.id, entry.depth);
    update();
}

QList<ArchipelagoEntry> PaperReadingArchipelago::entries() const { return entries_; }

int PaperReadingArchipelago::exploredCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.explored) c++;
    return c;
}

qreal PaperReadingArchipelago::avgDepth() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.depth;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingArchipelago::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingArchipelago::onExplore() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Theory", "Applied", "Survey", "Experimental"};
    QStringList regions = {"Pacific", "Atlantic", "Indian", "Arctic", "Southern"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ArchipelagoEntry e;
        e.id = entries_.size() + 1;
        e.island = text.left(8) + " island" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.region = regions[QRandomGenerator::global()->bounded(regions.size())];
        e.depth = 1 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.papers = 5 + QRandomGenerator::global()->bounded(50);
        e.explored = QRandomGenerator::global()->bounded(100) < 40;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingArchipelago::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Explore reading archipelago");
    update();
}

void PaperReadingArchipelago::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Explore reading archipelago");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Archipelago");
    int w = width(), h = height();
    drawIslandMap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingArchipelago::drawIslandMap(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
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
                   e.island.left(14) + (e.explored ? " [E]" : " [U]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.region + " | " + QString::number(e.papers) + " papers");
        int barW = static_cast<int>((e.depth / 5.0) * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22,
                   QString::number(e.depth, 'f', 1) + "d " + QString::number(e.papers) + "p");
    }
}

void PaperReadingArchipelago::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Theory", "Applied", "Survey", "Experimental"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingArchipelago::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Islands", QString::number(entries_.size()), QColor(59,130,246)},
        {"Explored", QString::number(exploredCount()), QColor(22,163,74)},
        {"Avg Depth", QString::number(avgDepth(), 'f', 1), QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperReadingArchipelago::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Explore reading archipelago"); return; }
    infoLabel_->setText(QString("%1 islands | %2 explored | depth %3")
        .arg(entries_.size()).arg(exploredCount()).arg(avgDepth(), 0, 'f', 1));
}

void PaperReadingArchipelago::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ArchipelagoEntry e;
        e.id = settings_.value("id").toInt();
        e.island = settings_.value("island").toString();
        e.category = settings_.value("category").toString();
        e.region = settings_.value("region").toString();
        e.depth = settings_.value("depth").toDouble();
        e.papers = settings_.value("papers").toInt();
        e.explored = settings_.value("explored").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingArchipelago::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("island", entries_[i].island);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("region", entries_[i].region);
        settings_.setValue("depth", entries_[i].depth);
        settings_.setValue("papers", entries_[i].papers);
        settings_.setValue("explored", entries_[i].explored);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
