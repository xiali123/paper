#include "tools/PaperQueryOptimizer.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperQueryOptimizer::PaperQueryOptimizer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "QueryOptimizer")
{
    setupUI();
    loadSettings();
}

void PaperQueryOptimizer::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* leftPanel = new QVBoxLayout();

    optimizeBtn_ = new QPushButton("Optimize");
    optimizeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(optimizeBtn_, &QPushButton::clicked, this, &PaperQueryOptimizer::onOptimize);
    leftPanel->addWidget(optimizeBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"search", "paper", "citation", "user", "analytics"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Query text...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; padding: 4px 12px; border-radius: 4px; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperQueryOptimizer::onClear);
    leftPanel->addWidget(clearBtn_);

    leftPanel->addStretch();

    infoLabel_ = new QLabel("Query Optimizer");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    mainLayout->addLayout(leftPanel, 1);

    mainLayout->addStretch(3);

    setMinimumSize(700, 480);
}

void PaperQueryOptimizer::addEntry(const QueryOptEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<QueryOptEntry> PaperQueryOptimizer::entries() const {
    return entries_;
}

int PaperQueryOptimizer::optimizedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.optimized) ++c;
    return c;
}

qreal PaperQueryOptimizer::avgSpeedup() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.speedup;
    return sum / entries_.size();
}

QMap<QString, int> PaperQueryOptimizer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperQueryOptimizer::onOptimize() {
    QString query = inputField_->text().trimmed();
    if (query.isEmpty()) return;

    static const QColor palette[] = {
        QColor(59, 130, 246),
        QColor(22, 163, 74),
        QColor(217, 119, 6),
        QColor(220, 38, 38),
        QColor(124, 58, 237)
    };

    QStringList plans = {"Index Scan", "Seq Scan", "Hash Join", "Merge Join", "Bitmap Scan"};

    int n = 4 + QRandomGenerator::global()->bounded(5);

    for (int i = 0; i < n; ++i) {
        QueryOptEntry entry;
        entry.id        = entries_.size() + 1;
        entry.query     = query;
        entry.category  = categoryCombo_->currentText();
        entry.plan      = plans[QRandomGenerator::global()->bounded(plans.size())];
        entry.speedup   = 1.0 + QRandomGenerator::global()->generateDouble() * 9.0;
        entry.beforeMs  = 10 + QRandomGenerator::global()->bounded(4990);
        entry.optimized = QRandomGenerator::global()->bounded(2) == 0;
        entry.color     = palette[QRandomGenerator::global()->bounded(5)];
        addEntry(entry);
        emit queryOptimized(entry.id, entry.speedup);
    }

    inputField_->clear();
}

void PaperQueryOptimizer::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperQueryOptimizer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Query Optimizer");
        return;
    }

    int w = width(), h = height();

    int colW = (w - 80) / 3;
    drawQueryView(p, QRect(20, 50, colW, h - 80));
    drawCategoryChart(p, QRect(30 + colW, 50, colW, h - 80));
    drawStats(p, QRect(40 + 2 * colW, 50, colW, h - 80));
}

void PaperQueryOptimizer::drawQueryView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "Query Performance");

    if (entries_.isEmpty()) return;

    int show = qMin(20, entries_.size());
    qreal maxSpd = 1.0;
    for (int i = 0; i < show; ++i)
        maxSpd = qMax(maxSpd, entries_[i].speedup);

    int chartTop = rect.y() + 30;
    int chartH   = rect.height() - 50;
    int barW     = qMax(6, (rect.width() - 10) / show - 2);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int barH = static_cast<int>((e.speedup / maxSpd) * chartH);
        int x    = rect.x() + 5 + i * (barW + 2);
        int y    = chartTop + chartH - barH;

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(x, y, barW, barH, 2, 2);

        if (barW >= 14) {
            p.save();
            p.translate(x + barW / 2, chartTop + chartH + 4);
            p.rotate(-45);
            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 6));
            p.drawText(0, 0, e.query.left(12));
            p.restore();
        }
    }

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(rect.x(), chartTop - 2, QString("%1x").arg(maxSpd, 0, 'f', 1));
    p.drawText(rect.x(), chartTop + chartH + 2, "1.0x");
}

void PaperQueryOptimizer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"search", "paper", "citation", "user", "analytics"};
    QString labels[] = {"Search", "Paper", "Citation", "User", "Analytics"};
    QColor colors[] = {
        QColor(59, 130, 246),
        QColor(22, 163, 74),
        QColor(217, 119, 6),
        QColor(220, 38, 38),
        QColor(124, 58, 237)
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int chartTop = rect.y() + 34;
    int barH     = qMin(24, (rect.height() - 50) / 5);
    int maxBarW  = rect.width() - 100;

    for (int i = 0; i < 5; ++i) {
        int y     = chartTop + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int bw    = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y + 2, bw, barH - 4, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + 64 + bw, y + 2, 30, barH - 4, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperQueryOptimizer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Queries",    QString::number(entries_.size()),                QColor(59, 130, 246)},
        {"Optimized",  QString::number(optimizedCount()),              QColor(22, 163, 74)},
        {"Avg Speedup", QString::number(avgSpeedup(), 'f', 1) + "x",   QColor(217, 119, 6)}
    };

    int boxH = qMin(60, (rect.height() - 20) / 3);

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(rect.x() + 12, y + 6, rect.width() - 24, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + boxH / 2, rect.width() - 24, boxH / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperQueryOptimizer::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Query Optimizer");
        return;
    }
    infoLabel_->setText(QString("Queries: %1 | Optimized: %2 | Avg: %3x")
        .arg(entries_.size())
        .arg(optimizedCount())
        .arg(avgSpeedup(), 0, 'f', 1));
}

void PaperQueryOptimizer::loadSettings() {
    settings_.beginGroup("QueryOptimizer");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        QueryOptEntry e;
        e.id        = settings_.value("id").toInt();
        e.query     = settings_.value("query").toString();
        e.category  = settings_.value("category").toString();
        e.plan      = settings_.value("plan").toString();
        e.speedup   = settings_.value("speedup").toDouble();
        e.beforeMs  = settings_.value("beforeMs").toInt();
        e.optimized = settings_.value("optimized").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperQueryOptimizer::saveSettings() {
    settings_.beginGroup("QueryOptimizer");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("query",     entries_[i].query);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("plan",      entries_[i].plan);
        settings_.setValue("speedup",   entries_[i].speedup);
        settings_.setValue("beforeMs",  entries_[i].beforeMs);
        settings_.setValue("optimized", entries_[i].optimized);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
