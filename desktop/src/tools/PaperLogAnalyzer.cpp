#include "tools/PaperLogAnalyzer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLogAnalyzer::PaperLogAnalyzer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogAnalyzer")
{
    setupUI();
    loadSettings();
}

void PaperLogAnalyzer::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperLogAnalyzer::onAnalyze);
    toolbar->addWidget(analyzeBtn_);
    toolbar->addWidget(new QLabel("Level:"));
    levelCombo_ = new QComboBox();
    levelCombo_->addItems({"All", "ERROR", "WARN", "INFO", "DEBUG"});
    toolbar->addWidget(levelCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogAnalyzer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter log source to analyze...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Analyze application logs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperLogAnalyzer::addEntry(const LogEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit logAnalyzed(entry.id, entry.frequency);
    update();
}

QList<LogEntry> PaperLogAnalyzer::entries() const { return entries_; }

qreal PaperLogAnalyzer::avgFrequency() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.frequency;
    return sum / entries_.size();
}

int PaperLogAnalyzer::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

QMap<QString, int> PaperLogAnalyzer::levelCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.level]++;
    return counts;
}

void PaperLogAnalyzer::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList levels = {"ERROR", "WARN", "INFO", "DEBUG"};
    QStringList sources = {"api", "database", "cache", "network", "renderer"};
    QStringList messages = {"connection timeout", "rate limit exceeded", "cache miss", "auth failed", "slow query", "retry success"};
    QStringList categories = {"infrastructure", "application", "security", "performance"};
    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        LogEntry e;
        e.id = entries_.size() + 1;
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
        e.message = messages[QRandomGenerator::global()->bounded(messages.size())];
        e.count = 1 + QRandomGenerator::global()->bounded(100);
        e.timestamp = "2026-05-10 " + QString::number(QRandomGenerator::global()->bounded(24)).rightJustified(2, '0') + ":" +
                      QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0');
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.frequency = e.count / 24.0;
        e.critical = e.level == "ERROR";
        e.color = e.level == "ERROR" ? QColor(239,68,68) : (e.level == "WARN" ? QColor(245,158,11) :
                   (e.level == "INFO" ? QColor(59,130,246) : QColor(156,163,175)));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLogAnalyzer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Analyze application logs");
    update();
}

void PaperLogAnalyzer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Analyze application logs");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Log Analyzer");
    int w = width(), h = height();
    drawLogList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLevelChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLogAnalyzer::drawLogList(QPainter& p, const QRect& rect) {
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
                   e.message.left(24));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   "x" + QString::number(e.count));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.timestamp);
    }
}

void PaperLogAnalyzer::drawLevelChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Log Levels");
    auto counts = levelCounts();
    QStringList levels = {"ERROR", "WARN", "INFO", "DEBUG"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246), QColor(156,163,175)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(levels[i]) ? counts[levels[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }
    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperLogAnalyzer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Log Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical", QString::number(criticalCount()), QColor(239,68,68)},
        {"Avg Freq", QString::number(avgFrequency(), 'f', 1) + "/hr", QColor(245,158,11)},
        {"Levels", QString::number(levelCounts().size()), QColor(139,92,246)}
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

void PaperLogAnalyzer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Analyze application logs"); return; }
    infoLabel_->setText(QString("%1 logs | %2 critical | %3/hr avg")
        .arg(entries_.size()).arg(criticalCount()).arg(avgFrequency(), 0, 'f', 1));
}

void PaperLogAnalyzer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.level = settings_.value("level").toString();
        e.message = settings_.value("message").toString();
        e.count = settings_.value("count").toInt();
        e.timestamp = settings_.value("timestamp").toString();
        e.category = settings_.value("category").toString();
        e.frequency = settings_.value("frequency").toDouble();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLogAnalyzer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("level", entries_[i].level);
        settings_.setValue("message", entries_[i].message);
        settings_.setValue("count", entries_[i].count);
        settings_.setValue("timestamp", entries_[i].timestamp);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("frequency", entries_[i].frequency);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
