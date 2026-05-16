#include "tools/PaperLogAnalyzer2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLogAnalyzer2::PaperLogAnalyzer2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogAnalyzer2")
{
    setupUI();
    loadSettings();
}

void PaperLogAnalyzer2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperLogAnalyzer2::onAnalyze);
    toolbar->addWidget(analyzeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "network", "database", "security", "performance", "rendering"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogAnalyzer2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter log source to analyze...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Analyze log patterns");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperLogAnalyzer2::addEntry(const LogEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit patternFound(entry.id, entry.frequency);
    update();
}

QList<LogEntry> PaperLogAnalyzer2::entries() const { return entries_; }

int PaperLogAnalyzer2::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

qreal PaperLogAnalyzer2::avgFrequency() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.frequency;
    return sum / entries_.size();
}

QMap<QString, int> PaperLogAnalyzer2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLogAnalyzer2::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList levels = {"ERROR", "WARN", "INFO", "DEBUG"};
    QStringList sources = {"api", "database", "cache", "network", "renderer"};
    QStringList categories = {"network", "database", "security", "performance", "rendering"};
    int catIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        LogEntry e;
        e.id = entries_.size() + 1;
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
        e.category = catIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[catIdx - 1];
        e.occurrences = 1 + QRandomGenerator::global()->bounded(100);
        e.frequency = e.occurrences / 24.0;
        e.critical = e.level == "ERROR";
        e.color = e.level == "ERROR" ? QColor(220,38,38) : (e.level == "WARN" ? QColor(217,119,6) :
                   (e.level == "INFO" ? QColor(59,130,246) : QColor(124,58,237)));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLogAnalyzer2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze log patterns");
    update();
}

void PaperLogAnalyzer2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze log patterns");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Log Pattern Analyzer");
    int w = width(), h = height();
    drawLogChart(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLogAnalyzer2::drawLogChart(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   "[" + e.level + "] " + e.source.left(8));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "x" + QString::number(e.occurrences));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.frequency, 'f', 1) + "/hr");
    }
}

void PaperLogAnalyzer2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"network", "database", "security", "performance", "rendering"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(22, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i].left(10));
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperLogAnalyzer2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical", QString::number(criticalCount()), QColor(220,38,38)},
        {"Avg Freq", QString::number(avgFrequency(), 'f', 1) + "/hr", QColor(217,119,6)},
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

void PaperLogAnalyzer2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze log patterns"); return; }
    infoLabel_->setText(QString("%1 entries | %2 critical | %3/hr avg | %4 categories")
        .arg(entries_.size()).arg(criticalCount()).arg(avgFrequency(), 0, 'f', 1).arg(categoryCounts().size()));
}

void PaperLogAnalyzer2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.category = settings_.value("category").toString();
        e.level = settings_.value("level").toString();
        e.frequency = settings_.value("frequency").toDouble();
        e.occurrences = settings_.value("occurrences").toInt();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLogAnalyzer2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("level", entries_[i].level);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("occurrences", entries_[i].occurrences);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
