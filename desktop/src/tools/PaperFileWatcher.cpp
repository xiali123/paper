#include "tools/PaperFileWatcher.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>

PaperFileWatcher::PaperFileWatcher(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FileWatcher")
{
    setupUI();
    loadSettings();
}

void PaperFileWatcher::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    watchBtn_ = new QPushButton("Watch");
    watchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(watchBtn_, &QPushButton::clicked, this, &PaperFileWatcher::onWatch);
    toolbar->addWidget(watchBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"Source", "Data", "Config", "Output", "Log"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFileWatcher::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter file path or pattern to watch...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Watch files");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperFileWatcher::addEntry(const WatchEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<WatchEntry> PaperFileWatcher::entries() const { return entries_; }

int PaperFileWatcher::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperFileWatcher::avgFrequency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.frequency;
    return sum / entries_.size();
}

QMap<QString, int> PaperFileWatcher::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFileWatcher::onWatch() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList events = {"created", "modified", "deleted", "renamed"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    WatchEntry e;
    e.id = entries_.size() + 1;
    e.path = text;
    e.category = categoryCombo_->currentText().toLower();
    e.event = events[QRandomGenerator::global()->bounded(events.size())];
    e.count = 1 + QRandomGenerator::global()->bounded(20);
    e.frequency = 0.5 + QRandomGenerator::global()->bounded(100) / 10.0;
    e.lastSeen = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    e.active = true;
    int cIdx = categoryCombo_->currentIndex();
    e.color = colors[cIdx % 5];
    addEntry(e);
    emit fileWatched(e.id, e.frequency);
    inputField_->clear();
}

void PaperFileWatcher::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Watch files");
    update();
}

void PaperFileWatcher::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Watch files");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "File Watcher");
    int w = width(), h = height();
    drawWatchList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFileWatcher::drawWatchList(QPainter& p, const QRect& rect) {
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
                   e.path.left(20));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.event + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "x" + QString::number(e.count) + " " + QString::number(e.frequency, 'f', 1) + "/s");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.active ? "ACTIVE" : "IDLE");
    }
}

void PaperFileWatcher::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList keys;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        keys << it.key();
    if (keys.isEmpty()) return;
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / qMax(keys.size(), 1));
    for (int i = 0; i < keys.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts[keys[i]];
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, keys[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i % 5]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperFileWatcher::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(22,163,74)},
        {"Avg Freq", QString::number(avgFrequency(), 'f', 1) + "/s", QColor(217,119,6)},
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

void PaperFileWatcher::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Watch files"); return; }
    infoLabel_->setText(QString("%1 watches | %2 active | %3 Hz avg")
        .arg(entries_.size()).arg(activeCount()).arg(avgFrequency(), 0, 'f', 1));
}

void PaperFileWatcher::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        WatchEntry e;
        e.id = settings_.value("id").toInt();
        e.path = settings_.value("path").toString();
        e.category = settings_.value("category").toString();
        e.event = settings_.value("event").toString();
        e.count = settings_.value("count").toInt();
        e.frequency = settings_.value("frequency").toDouble();
        e.lastSeen = settings_.value("lastSeen").toString();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFileWatcher::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("path", entries_[i].path);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("event", entries_[i].event);
        settings_.setValue("count", entries_[i].count);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("lastSeen", entries_[i].lastSeen);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
