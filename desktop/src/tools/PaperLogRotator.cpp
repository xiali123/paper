#include "tools/PaperLogRotator.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLogRotator::PaperLogRotator(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LogRotator")
{
    setupUI();
    loadSettings();
}

void PaperLogRotator::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    rotateBtn_ = new QPushButton("Rotate");
    rotateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(rotateBtn_, &QPushButton::clicked, this, &PaperLogRotator::onRotate);
    toolbar->addWidget(rotateBtn_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLogRotator::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "system", "network", "database", "ui", "security"});
    toolbar->addWidget(categoryCombo_, 1);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter source name to rotate...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Rotate logs");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperLogRotator::addEntry(const LogEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<LogEntry> PaperLogRotator::entries() const {
    return entries_;
}

int PaperLogRotator::compressedCount() const {
    int c = 0;
    for (const auto& e : entries_) {
        if (e.compressed) c++;
    }
    return c;
}

qreal PaperLogRotator::avgSize() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) {
        total += e.size;
    }
    return total / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperLogRotator::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperLogRotator::onRotate() {
    QString source = inputField_->text().trimmed();
    if (source.isEmpty()) return;

    QStringList levels = {"info", "warning", "error", "debug"};
    QStringList categories = {"system", "network", "database", "ui", "security"};
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int catIdx = categoryCombo_->currentIndex();
    QString category = catIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[catIdx - 1];

    LogEntry e;
    e.id = entries_.size() + 1;
    e.source = source;
    e.category = category;
    e.level = levels[QRandomGenerator::global()->bounded(levels.size())];
    e.count = 1 + QRandomGenerator::global()->bounded(100);
    e.size = QRandomGenerator::global()->generateDouble() * 50.0 + 0.5;
    e.lastRotate = "2026-05-12 "
                 + QString::number(QRandomGenerator::global()->bounded(24)).rightJustified(2, '0') + ":"
                 + QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0') + ":"
                 + QString::number(QRandomGenerator::global()->bounded(60)).rightJustified(2, '0');
    e.compressed = QRandomGenerator::global()->bounded(2) == 0;
    e.color = palette[QRandomGenerator::global()->bounded(5)];

    qreal sz = e.size;
    int id = e.id;
    addEntry(e);
    emit logRotated(id, sz);
    inputField_->clear();
}

void PaperLogRotator::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Rotate logs");
    update();
}

void PaperLogRotator::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Rotate logs");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Log Rotator");

    int w = width(), h = height();
    drawLogList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLogRotator::drawLogList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.source.left(14) + (e.compressed ? " [Z]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.level.toUpper() + " | cnt:" + QString::number(e.count)
                   + " | " + QString::number(e.size, 'f', 1) + "MB");

        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.lastRotate.mid(11));
    }
}

void PaperLogRotator::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList catKeys = counts.keys();
    if (catKeys.isEmpty()) return;

    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(24, (rect.height() - 30) / qMax(catKeys.size(), 1));
    for (int i = 0; i < catKeys.size(); ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts[catKeys[i]];
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, catKeys[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(palette[i % 5]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperLogRotator::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Logs",   QString::number(entries_.size()),   QColor(59,130,246)},
        {"Compressed",   QString::number(compressedCount()), QColor(22,163,74)},
        {"Avg Size",     QString::number(avgSize(), 'f', 1) + " MB", QColor(217,119,6)},
        {"Categories",   QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperLogRotator::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Rotate logs");
        return;
    }
    infoLabel_->setText(QString("%1 logs | %2 compressed | avg %3 MB")
        .arg(entries_.size())
        .arg(compressedCount())
        .arg(avgSize(), 0, 'f', 1));
}

void PaperLogRotator::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LogEntry e;
        e.id         = settings_.value("id").toInt();
        e.source     = settings_.value("source").toString();
        e.category   = settings_.value("category").toString();
        e.level      = settings_.value("level").toString();
        e.count      = settings_.value("count").toInt();
        e.size       = settings_.value("size").toDouble();
        e.lastRotate = settings_.value("lastRotate").toString();
        e.compressed = settings_.value("compressed").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLogRotator::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",         entries_[i].id);
        settings_.setValue("source",     entries_[i].source);
        settings_.setValue("category",   entries_[i].category);
        settings_.setValue("level",      entries_[i].level);
        settings_.setValue("count",      entries_[i].count);
        settings_.setValue("size",       entries_[i].size);
        settings_.setValue("lastRotate", entries_[i].lastRotate);
        settings_.setValue("compressed", entries_[i].compressed);
        settings_.setValue("color",      entries_[i].color.name());
    }
    settings_.endArray();
}
