#include "tools/PaperGapFinder2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <numeric>

PaperGapFinder2::PaperGapFinder2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "GapFinder2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList domains = {"Machine Learning", "NLP", "Computer Vision", "Robotics"};
        QStringList categories = {"Research", "Industry", "Education", "Policy", "Practice"};
        QStringList types = {"Knowledge", "Method", "Data", "Theory", "Application"};
        QColor palette[] = {
            QColor(59, 130, 246), QColor(22, 163, 74),
            QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
        };

        for (int i = 0; i < 8; ++i) {
            GapFinder2Entry e;
            e.id = i + 1;
            e.domain = domains[i % domains.size()];
            e.category = categories[i % categories.size()];
            e.type = types[i % types.size()];
            e.severity = QRandomGenerator::global()->bounded(20, 100) / 100.0;
            e.gaps = QRandomGenerator::global()->bounded(1, 15);
            e.critical = e.severity > 0.7;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperGapFinder2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Industry", "Education", "Policy", "Practice"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this]() { update(); });
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search domains...");
    toolbar->addWidget(inputField_, 1);

    findBtn_ = new QPushButton("Find");
    findBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(findBtn_, &QPushButton::clicked, this, &PaperGapFinder2::onFind);
    toolbar->addWidget(findBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperGapFinder2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    infoLabel_ = new QLabel();
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbar->addWidget(infoLabel_);

    layout->addLayout(toolbar);
    setMinimumSize(680, 520);
}

void PaperGapFinder2::addEntry(const GapFinder2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit gapFound(entry.id, entry.severity);
    update();
}

QList<GapFinder2Entry> PaperGapFinder2::entries() const { return entries_; }

int PaperGapFinder2::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.critical) ++c;
    return c;
}

qreal PaperGapFinder2::avgSeverity() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.severity;
    return sum / entries_.size();
}

QMap<QString, int> PaperGapFinder2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperGapFinder2::onFind() {
    QStringList domains = {"Machine Learning", "NLP", "Computer Vision", "Robotics"};
    QStringList categories = {"Research", "Industry", "Education", "Policy", "Practice"};
    QStringList types = {"Knowledge", "Method", "Data", "Theory", "Application"};
    QColor palette[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
    };

    GapFinder2Entry e;
    e.id = entries_.size() + 1;
    e.domain = domains[QRandomGenerator::global()->bounded(domains.size())];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.type = types[QRandomGenerator::global()->bounded(types.size())];
    e.severity = QRandomGenerator::global()->bounded(20, 100) / 100.0;
    e.gaps = QRandomGenerator::global()->bounded(1, 15);
    e.critical = e.severity > 0.7;
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperGapFinder2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("No gaps found");
    update();
}

void PaperGapFinder2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Find research gaps across domains");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Gap Finder");

    int w = width(), h = height();
    int contentTop = 45;

    int viewW = static_cast<int>(w * 0.6) - 25;
    int chartX = static_cast<int>(w * 0.6) + 5;
    int chartW = static_cast<int>(w * 0.4) - 25;
    int topH = static_cast<int>(h * 0.75) - contentTop;
    int statsY = static_cast<int>(h * 0.75) + 5;
    int statsH = h - statsY - 15;

    drawFinderView(p, QRect(15, contentTop, viewW, topH));
    drawCategoryChart(p, QRect(chartX, contentTop, chartW, topH));
    drawStats(p, QRect(15, statsY, w - 30, statsH));
}

void PaperGapFinder2::drawFinderView(QPainter& p, const QRect& area) {
    QString filter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();

    QList<const GapFinder2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter != "All" && e.category != filter) continue;
        if (!search.isEmpty() && !e.domain.toLower().contains(search)) continue;
        visible.append(&e);
    }

    int cardH = qMin(52, (area.height() - 10) / qMax(1, visible.size()));
    int show = qMin(visible.size(), (area.height() - 10) / qMax(1, cardH + 4));

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[visible.size() - 1 - i];
        int y = area.y() + i * (cardH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(area.x(), y, area.width(), cardH, 6, 6);

        // Domain name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(area.x() + 10, y + 3, area.width() - 20, 18, Qt::AlignVCenter,
                   e.domain);

        // Type badge
        QFontMetrics fmBadge(QFont("Arial", 8));
        int badgeW = fmBadge.horizontalAdvance(e.type) + 14;
        int badgeX = area.x() + 10;
        int badgeY = y + 22;

        QColor badgeBg = e.color;
        badgeBg.setAlpha(40);
        p.setBrush(badgeBg);
        p.drawRoundedRect(badgeX, badgeY, badgeW, 16, 3, 3);
        p.setPen(e.color);
        p.setFont(QFont("Arial", 8));
        p.drawText(badgeX + 7, badgeY, badgeW - 14, 16, Qt::AlignVCenter, e.type);

        // Severity bar
        int barX = badgeX + badgeW + 12;
        int barW = qMin(100, area.width() - badgeW - 100);
        int barH = 8;
        int barY = badgeY + 4;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        QColor sevColor;
        if (e.severity > 0.7)      sevColor = QColor(220, 38, 38);
        else if (e.severity > 0.4) sevColor = QColor(217, 119, 6);
        else                       sevColor = QColor(22, 163, 74);
        int fillW = static_cast<int>(e.severity * barW);
        p.setBrush(sevColor);
        p.drawRoundedRect(barX, barY, fillW, barH, 3, 3);

        // Severity value text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, barY + barH,
                   QString::number(e.severity, 'f', 2));

        // Gap count (right side)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 11, QFont::Bold));
        p.drawText(area.x() + area.width() - 60, y + 2, 30, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.gaps));
        p.setFont(QFont("Arial", 7));
        p.setPen(QColor(100, 116, 139));
        p.drawText(area.x() + area.width() - 60, y + 20, 30, 14,
                   Qt::AlignVCenter | Qt::AlignRight, "gaps");

        // Critical indicator
        if (e.critical) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38));
            p.drawEllipse(area.x() + area.width() - 20, y + cardH / 2 - 5, 10, 10);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(area.x() + area.width() - 20, y + cardH / 2 - 5, 10, 10,
                       Qt::AlignCenter, "!");
        }
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(area, Qt::AlignCenter, "No matching gaps");
    }
}

static int fmAdvance(const QFontMetrics& fm, const QString& text) {
    return fm.horizontalAdvance(text);
}

void PaperGapFinder2::drawCategoryChart(QPainter& p, const QRect& area) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(area.topLeft(), "Type Distribution");

    QStringList types = {"Knowledge", "Method", "Data", "Theory", "Application"};
    QColor typeColors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74),
        QColor(217, 119, 6), QColor(220, 38, 38), QColor(124, 58, 237)
    };

    // Count types per category
    QStringList categories = {"Research", "Industry", "Education", "Policy", "Practice"};
    QMap<QString, QMap<QString, int>> data;
    for (const auto& e : entries_) {
        data[e.category][e.type]++;
    }

    int chartTop = area.y() + 22;
    int chartH = area.height() - 50;
    int catCount = categories.size();
    int barGroupW = qMin(50, (area.width() - 10) / catCount - 6);

    // Find max total per category for scaling
    int maxTotal = 1;
    for (const auto& cat : categories) {
        int total = 0;
        for (const auto& t : types) total += data[cat].value(t, 0);
        maxTotal = qMax(maxTotal, total);
    }

    for (int c = 0; c < catCount; ++c) {
        int x = area.x() + c * (barGroupW + 6) + 4;
        int barBottom = chartTop + chartH;
        int yOffset = 0;

        for (int t = 0; t < types.size(); ++t) {
            int count = data[categories[c]].value(types[t], 0);
            int segH = static_cast<int>(
                (static_cast<qreal>(count) / maxTotal) * chartH);

            if (segH > 0) {
                p.setPen(Qt::NoPen);
                p.setBrush(typeColors[t]);
                p.drawRoundedRect(x, barBottom - yOffset - segH, barGroupW, segH, 2, 2);
                yOffset += segH;
            }
        }

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 2, barBottom + 4, barGroupW + 4, 14,
                   Qt::AlignHCenter | Qt::AlignTop,
                   categories[c].left(5));
    }

    // Legend at bottom
    int legendY = chartTop + chartH + 20;
    int legendX = area.x();
    p.setFont(QFont("Arial", 7));
    for (int t = 0; t < types.size(); ++t) {
        p.setPen(Qt::NoPen);
        p.setBrush(typeColors[t]);
        p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);
        p.setPen(QColor(100, 116, 139));
        p.drawText(legendX + 13, legendY + 10, types[t]);
        legendX += fmAdvance(p.fontMetrics(), types[t]) + 22;
    }
}

void PaperGapFinder2::drawStats(QPainter& p, const QRect& area) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Domains",  QString::number(entries_.size()),   QColor(59, 130, 246)},
        {"Critical Count", QString::number(criticalCount()),   QColor(220, 38, 38)},
        {"Avg Severity",   QString::number(avgSeverity(), 'f', 2), QColor(217, 119, 6)},
        {"Total Gaps",     QString::number(std::accumulate(
                               entries_.cbegin(), entries_.cend(), 0,
                               [](int s, const GapFinder2Entry& e) { return s + e.gaps; })),
                                                               QColor(124, 58, 237)}
    };

    int boxW = qMin(160, (area.width() - 30) / 4);
    int boxH = qMin(48, area.height() - 4);

    for (int i = 0; i < stats.size(); ++i) {
        int x = area.x() + i * (boxW + 10);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, area.y(), boxW, boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, area.y() + 4, boxW - 20, 24,
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 10, area.y() + 28, boxW - 20, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperGapFinder2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("No gaps found");
        return;
    }
    infoLabel_->setText(
        QString("%1 domains | %2 critical | avg severity %3 | %4 gaps")
            .arg(entries_.size())
            .arg(criticalCount())
            .arg(avgSeverity(), 0, 'f', 2)
            .arg(std::accumulate(entries_.cbegin(), entries_.cend(), 0,
                                 [](int s, const GapFinder2Entry& e) { return s + e.gaps; })));
}

void PaperGapFinder2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GapFinder2Entry e;
        e.id       = settings_.value("id").toInt();
        e.domain   = settings_.value("domain").toString();
        e.category = settings_.value("category").toString();
        e.type     = settings_.value("type").toString();
        e.severity = settings_.value("severity").toDouble();
        e.gaps     = settings_.value("gaps").toInt();
        e.critical = settings_.value("critical").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperGapFinder2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("domain",   entries_[i].domain);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type",     entries_[i].type);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("gaps",     entries_[i].gaps);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
