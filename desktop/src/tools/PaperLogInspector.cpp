#include "tools/PaperLogInspector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLogInspector::PaperLogInspector(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogInspector")
{
    setupUI();
    loadSettings();
}

void PaperLogInspector::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    captureBtn_ = new QPushButton("Capture");
    captureBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(captureBtn_, &QPushButton::clicked, this, &PaperLogInspector::onCapture);
    toolbar->addWidget(captureBtn_);
    toolbar->addWidget(new QLabel("Level:"));
    levelCombo_ = new QComboBox();
    levelCombo_->addItems({"All", "Error", "Warning", "Info", "Debug"});
    toolbar->addWidget(levelCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogInspector::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter log filter...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Inspect logs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperLogInspector::addEntry(const LogEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit logCaptured(entry.id, entry.level);
    update();
}

QList<LogEntry> PaperLogInspector::entries() const { return entries_; }

int PaperLogInspector::errorCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.error) c++;
    return c;
}

QMap<QString, int> PaperLogInspector::levelCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.level]++;
    return counts;
}

QMap<QString, int> PaperLogInspector::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLogInspector::onCapture() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList levels = {"error", "warning", "info", "debug"};
    QStringList sources = {"crawler", "parser", "indexer", "renderer", "api"};
    QStringList categories = {"network", "database", "ui", "system"};
    int lIdx = levelCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        LogEntry e;
        e.id = entries_.size() + 1;
        e.message = text.left(10) + " msg" + QString::number(i);
        e.level = lIdx == 0 ? levels[QRandomGenerator::global()->bounded(levels.size())] : levels[lIdx - 1];
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.timestamp = "2026-05-10 " + QString::number(QRandomGenerator::global()->bounded(24)).rightJustified(2, '0') + ":"
                    + QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0') + ":"
                    + QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0');
        e.error = e.level == "error";
        e.color = e.error ? QColor(239,68,68) : (e.level == "warning" ? QColor(245,158,11) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLogInspector::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Inspect logs");
    update();
}

void PaperLogInspector::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Inspect logs");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Log Inspector");
    int w = width(), h = height();
    drawLogList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLevelChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLogInspector::drawLogList(QPainter& p, const QRect& rect) {
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
                   e.message.left(14) + (e.error ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.source + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.level.toUpper());
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.timestamp.mid(11));
    }
}

void PaperLogInspector::drawLevelChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Levels");
    auto counts = levelCounts();
    QStringList levels = {"error", "warning", "info", "debug"};
    QString labels[] = {"Error", "Warning", "Info", "Debug"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246), QColor(16,185,129)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(levels[i]) ? counts[levels[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperLogInspector::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Logs", QString::number(entries_.size()), QColor(59,130,246)},
        {"Errors", QString::number(errorCount()), QColor(239,68,68)},
        {"Levels", QString::number(levelCounts().size()), QColor(245,158,11)},
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

void PaperLogInspector::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Inspect logs"); return; }
    infoLabel_->setText(QString("%1 logs | %2 errors | %3 levels")
        .arg(entries_.size()).arg(errorCount()).arg(levelCounts().size()));
}

void PaperLogInspector::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogEntry e;
        e.id = settings_.value("id").toInt();
        e.message = settings_.value("message").toString();
        e.level = settings_.value("level").toString();
        e.source = settings_.value("source").toString();
        e.category = settings_.value("category").toString();
        e.timestamp = settings_.value("timestamp").toString();
        e.error = settings_.value("error").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLogInspector::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("message", entries_[i].message);
        settings_.setValue("level", entries_[i].level);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("timestamp", entries_[i].timestamp);
        settings_.setValue("error", entries_[i].error);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
