#include "tools/PaperLogViewer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLogViewer::PaperLogViewer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogViewer")
{
    setupUI();
    loadSettings();
}

void PaperLogViewer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    fetchBtn_ = new QPushButton("Fetch");
    fetchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(fetchBtn_, &QPushButton::clicked, this, &PaperLogViewer::onFetch);
    toolbar->addWidget(fetchBtn_);

    toolbar->addWidget(new QLabel("Level:"));
    levelCombo_ = new QComboBox();
    levelCombo_->addItems({"All", "Error", "Warning", "Info", "Debug"});
    toolbar->addWidget(levelCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogViewer::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter filter keyword...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("View application logs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperLogViewer::addEntry(const LogEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit logAdded(entry.id, entry.level);
    update();
}

QList<LogEntry> PaperLogViewer::entries() const { return entries_; }

int PaperLogViewer::errorCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.isError) c++;
    return c;
}

int PaperLogViewer::warningCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.level == "warning") c++;
    return c;
}

QMap<QString, int> PaperLogViewer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLogViewer::onFetch() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList levels = {"error", "warning", "info", "debug"};
    QStringList sources = {"crawler", "parser", "indexer", "api", "database"};
    QStringList categories = {"search", "paper", "citation", "export", "system"};
    QStringList timestamps = {"10:30:15", "10:30:16", "10:30:17", "10:30:18", "10:30:19"};

    int count = 5 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        LogEntry e;
        e.id = entries_.size() + 1;
        e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.timestamp = timestamps[i % timestamps.size()];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.line = 10 + QRandomGenerator::global()->bounded(500);
        e.message = (text.left(10) + " " + e.source + " " + e.category + " op" + QString::number(i)).left(30);
        e.isError = e.level == "error";
        e.color = e.level == "error" ? QColor(239,68,68) :
                  (e.level == "warning" ? QColor(245,158,11) :
                  (e.level == "info" ? QColor(59,130,246) : QColor(100,116,139)));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperLogViewer::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("View application logs");
    update();
}

void PaperLogViewer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "View application logs");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Log Viewer");

    int w = width(), h = height();
    drawLogList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLevelChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLogViewer::drawLogList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(195));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.message.left(22));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.source + " | L" + QString::number(e.line) + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.level.toUpper());
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.timestamp);
    }
}

void PaperLogViewer::drawLevelChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Levels");

    QMap<QString, int> levelMap;
    for (const auto& e : entries_) levelMap[e.level]++;
    QStringList levels = {"error", "warning", "info", "debug"};
    QString labels[] = {"Error", "Warning", "Info", "Debug"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246), QColor(100,116,139)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = levelMap.contains(levels[i]) ? levelMap[levels[i]] : 0;
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

void PaperLogViewer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Errors", QString::number(errorCount()), QColor(239,68,68)},
        {"Warnings", QString::number(warningCount()), QColor(245,158,11)},
        {"Sources", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperLogViewer::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("View application logs"); return; }
    infoLabel_->setText(QString("%1 logs | %2 errors | %3 warnings")
        .arg(entries_.size()).arg(errorCount()).arg(warningCount()));
}

void PaperLogViewer::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogEntry e;
        e.id = settings_.value("id").toInt();
        e.message = settings_.value("message").toString();
        e.level = settings_.value("level").toString();
        e.source = settings_.value("source").toString();
        e.timestamp = settings_.value("timestamp").toString();
        e.category = settings_.value("category").toString();
        e.line = settings_.value("line").toInt();
        e.isError = settings_.value("isError").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLogViewer::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("message", entries_[i].message);
        settings_.setValue("level", entries_[i].level);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("timestamp", entries_[i].timestamp);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("line", entries_[i].line);
        settings_.setValue("isError", entries_[i].isError);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
