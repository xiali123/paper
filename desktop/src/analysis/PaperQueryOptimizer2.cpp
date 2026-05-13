#include "analysis/PaperQueryOptimizer2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtNumeric>

namespace {
static const QColor kBlue   = QColor(0x3b82f6);
static const QColor kGreen  = QColor(0x16a34a);
static const QColor kAmber  = QColor(0xd97706);
static const QColor kRed    = QColor(0xdc2626);
static const QColor kPurple = QColor(0x7c3aed);

static const QList<QColor> kPalette = { kBlue, kGreen, kAmber, kRed, kPurple };

static const QStringList kQueries = {
    "SELECT * FROM papers",
    "JOIN citations ON paper_id",
    "FULLTEXT SEARCH title abstract",
    "SELECT COUNT(*) FROM refs",
    "INSERT INTO bookmarks VALUES",
    "UPDATE papers SET rating =",
    "DELETE FROM temp_cache WHERE",
    "CREATE MATERIALIZED VIEW top_cited AS"
};

static const QStringList kStrategies = {
    "Index Scan", "Hash Join", "Merge Sort", "Bitmap", "Nested Loop"
};

static const QStringList kCategories = {
    "OLTP", "OLAP", "Full-text", "Graph", "Time-series"
};
}

PaperQueryOptimizer2::PaperQueryOptimizer2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "QueryOptimizer2")
{
    setupUI();
    loadSettings();
}

void PaperQueryOptimizer2::setupUI() {
    auto* layout = new QHBoxLayout(this);

    auto* left = new QHBoxLayout();
    left->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems(kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter SQL query...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    left->addWidget(inputField_);

    optimizeBtn_ = new QPushButton("Optimize");
    optimizeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(optimizeBtn_, &QPushButton::clicked, this, &PaperQueryOptimizer2::onOptimize);
    left->addWidget(optimizeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperQueryOptimizer2::onClear);
    left->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Queries: 0 | Optimal: 0 | Avg Speedup: 0.00x");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    left->addWidget(infoLabel_);

    layout->addLayout(left);
    layout->addStretch();

    setMinimumSize(640, 520);
}

void PaperQueryOptimizer2::addEntry(const QueryOptimizer2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<QueryOptimizer2Entry> PaperQueryOptimizer2::entries() const {
    return entries_;
}

int PaperQueryOptimizer2::optimalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.optimal) ++c;
    return c;
}

qreal PaperQueryOptimizer2::avgSpeedup() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.speedup;
    return sum / entries_.size();
}

QMap<QString, int> PaperQueryOptimizer2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperQueryOptimizer2::onOptimize() {
    QString query = inputField_->text().trimmed();
    if (query.isEmpty()) {
        query = kQueries[QRandomGenerator::global()->bounded(kQueries.size())];
    }

    QString category = categoryCombo_->currentText();
    QString strategy = kStrategies[QRandomGenerator::global()->bounded(kStrategies.size())];

    qreal speedup = 1.0 + QRandomGenerator::global()->generateDouble() * 3.0;
    int executions = QRandomGenerator::global()->bounded(1, 500);
    bool optimal = speedup > 2.0;

    QColor color = kPalette[entries_.size() % kPalette.size()];

    QueryOptimizer2Entry entry;
    entry.id = entries_.size() + 1;
    entry.query = query;
    entry.category = category;
    entry.strategy = strategy;
    entry.speedup = speedup;
    entry.executions = executions;
    entry.optimal = optimal;
    entry.color = color;

    addEntry(entry);
    emit queryOptimized(entry.id, entry.speedup);
    inputField_->clear();
}

void PaperQueryOptimizer2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperQueryOptimizer2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Optimize database queries");
        return;
    }

    int w = width(), h = height();
    int colW = (w - 50) / 3;
    drawOptimizerView(p, QRect(10, 10, colW, h - 20));
    drawCategoryChart(p, QRect(20 + colW, 10, colW, h - 20));
    drawStats(p, QRect(30 + colW * 2, 10, colW, h - 20));
}

void PaperQueryOptimizer2::drawOptimizerView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Query Optimizer");

    int show = qMin(8, entries_.size());
    int cardH = qMin(52, (rect.height() - 40) / qMax(show, 1));
    int startY = rect.y() + 35;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = startY + i * (cardH + 5);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(255, 255, 255));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 6, 6);

        // Left color bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, cardH, 2, 2);

        // Query text (truncated)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 12, y + 4, rect.width() - 24, 16, Qt::AlignVCenter,
                   e.query.left(30));

        // Strategy badge
        QColor badgeBg;
        if (e.speedup > 2.0)      badgeBg = kGreen;
        else if (e.speedup > 1.5) badgeBg = kAmber;
        else                      badgeBg = kRed;

        int badgeW = 70;
        int badgeX = rect.x() + 12;
        int badgeY = y + 22;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeBg);
        p.drawRoundedRect(badgeX, badgeY, badgeW, 16, 3, 3);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, badgeY, badgeW, 16, Qt::AlignCenter, e.strategy);

        // Speedup bar
        int barX = badgeX + badgeW + 8;
        int barW = static_cast<int>(qMin(e.speedup / 4.0, 1.0) * 80);
        int barY = badgeY + 3;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeBg.lighter(140));
        p.drawRoundedRect(barX, barY, barW, 10, 3, 3);
        p.setBrush(badgeBg);
        p.drawRoundedRect(barX, barY, barW, 10, 3, 3);

        // Speedup text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, barY + 9,
                   QString("%1x").arg(e.speedup, 0, 'f', 1));

        // Execution count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 60, badgeY + 12,
                   QString("Exec: %1").arg(e.executions));

        // Optimal checkmark
        if (e.optimal) {
            p.setPen(kGreen);
            p.setFont(QFont("Arial", 11, QFont::Bold));
            p.drawText(rect.x() + rect.width() - 18, y + 4, 16, 20,
                       Qt::AlignVCenter, QChar(0x2713));
        }
    }
}

void PaperQueryOptimizer2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 20, "Categories");

    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    QStringList categories = counts.keys();
    int maxVal = 1;
    for (int v : counts.values())
        maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 40) / qMax(categories.size(), 1));
    int startY = rect.y() + 38;

    for (int i = 0; i < categories.size(); ++i) {
        int y = startY + i * (barH + 4);
        int count = counts[categories[i]];
        int barW = static_cast<int>(
            (static_cast<qreal>(count) / maxVal) * (rect.width() - 120));

        // Category label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   categories[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i % kPalette.size()]);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        // Count label
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperQueryOptimizer2::drawStats(QPainter& p, const QRect& rect) {
    int totalExec = 0;
    for (const auto& e : entries_) totalExec += e.executions;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Queries",  QString::number(entries_.size()), kBlue},
        {"Optimal Count",  QString::number(optimalCount()),  kGreen},
        {"Avg Speedup",    QString("%1x").arg(avgSpeedup(), 0, 'f', 2), kAmber},
        {"Total Executions", QString::number(totalExec),     kPurple}
    };

    int boxH = qMin(55, (rect.height() - 20) / qMax(stats.size(), 1));
    int startY = rect.y() + 10;

    for (int i = 0; i < stats.size(); ++i) {
        int y = startY + i * (boxH + 6);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() - 20, 26, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 30, rect.width() - 20, 18, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperQueryOptimizer2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Queries: 0 | Optimal: 0 | Avg Speedup: 0.00x");
        return;
    }
    infoLabel_->setText(
        QString("Queries: %1 | Optimal: %2 | Avg Speedup: %3x")
            .arg(entries_.size())
            .arg(optimalCount())
            .arg(avgSpeedup(), 0, 'f', 2));
}

void PaperQueryOptimizer2::loadSettings() {
    settings_.beginGroup("QueryOptimizer2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        QueryOptimizer2Entry e;
        e.id         = settings_.value("id").toInt();
        e.query      = settings_.value("query").toString();
        e.category   = settings_.value("category").toString();
        e.strategy   = settings_.value("strategy").toString();
        e.speedup    = settings_.value("speedup").toDouble();
        e.executions = settings_.value("executions").toInt();
        e.optimal    = settings_.value("optimal").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperQueryOptimizer2::saveSettings() {
    settings_.beginGroup("QueryOptimizer2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",         entries_[i].id);
        settings_.setValue("query",      entries_[i].query);
        settings_.setValue("category",   entries_[i].category);
        settings_.setValue("strategy",   entries_[i].strategy);
        settings_.setValue("speedup",    entries_[i].speedup);
        settings_.setValue("executions", entries_[i].executions);
        settings_.setValue("optimal",    entries_[i].optimal);
        settings_.setValue("color",      entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
