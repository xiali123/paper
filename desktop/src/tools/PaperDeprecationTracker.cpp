#include "tools/PaperDeprecationTracker.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperDeprecationTracker::PaperDeprecationTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DeprecationTracker")
{
    setupUI();
    loadSettings();
}

void PaperDeprecationTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperDeprecationTracker::onTrack);
    toolbar->addWidget(trackBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDeprecationTracker::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "API", "Method", "Parameter", "Header", "Event"});
    toolbar->addWidget(categoryCombo_, 1);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter api->replacement (e.g. oldMethod->newMethod)...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Track deprecations");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperDeprecationTracker::addEntry(const DeprecationEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<DeprecationEntry> PaperDeprecationTracker::entries() const { return entries_; }

int PaperDeprecationTracker::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

qreal PaperDeprecationTracker::avgUsage() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) total += e.usage;
    return total / entries_.size();
}

QMap<QString, int> PaperDeprecationTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDeprecationTracker::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList parts = text.split("->");
    QString api = parts[0].trimmed();
    QString replacement = parts.size() > 1 ? parts[1].trimmed() : "N/A";
    QStringList categories = {"API", "Method", "Parameter", "Header", "Event"};
    QStringList deadlines = {"2026-06-01", "2026-07-15", "2026-08-30", "2026-09-15", "2026-12-01"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        DeprecationEntry e;
        e.id = entries_.size() + 1;
        e.api = api;
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.replacement = replacement;
        e.deadline = deadlines[QRandomGenerator::global()->bounded(deadlines.size())];
        e.usage = 5.0 + QRandomGenerator::global()->bounded(950) / 10.0;
        e.critical = e.usage > 50.0;
        e.color = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
        emit deprecationTracked(e.id, e.usage);
    }
    inputField_->clear();
}

void PaperDeprecationTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track deprecations");
    update();
}

void PaperDeprecationTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track deprecations");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Deprecation Tracker");
    int w = width(), h = height();
    drawDeprecationList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDeprecationTracker::drawDeprecationList(QPainter& p, const QRect& rect) {
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
                   e.api + (e.critical ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.replacement + " | " + e.deadline);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.usage, 'f', 1) + "%");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.critical ? "CRITICAL" : "normal");
    }
}

void PaperDeprecationTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"API", "Method", "Parameter", "Header", "Event"};
    QString labels[] = {"API", "Method", "Param", "Header", "Event"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperDeprecationTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Tracked", QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical", QString::number(criticalCount()), QColor(220,38,38)},
        {"Avg Usage", QString::number(avgUsage(), 'f', 1) + "%", QColor(217,119,6)},
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

void PaperDeprecationTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track deprecations"); return; }
    infoLabel_->setText(QString("%1 tracked | %2 critical | avg %3%")
        .arg(entries_.size()).arg(criticalCount()).arg(avgUsage(), 0, 'f', 1));
}

void PaperDeprecationTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DeprecationEntry e;
        e.id = settings_.value("id").toInt();
        e.api = settings_.value("api").toString();
        e.category = settings_.value("category").toString();
        e.replacement = settings_.value("replacement").toString();
        e.deadline = settings_.value("deadline").toString();
        e.usage = settings_.value("usage").toDouble();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDeprecationTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("api", entries_[i].api);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("replacement", entries_[i].replacement);
        settings_.setValue("deadline", entries_[i].deadline);
        settings_.setValue("usage", entries_[i].usage);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
