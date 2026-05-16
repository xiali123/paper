#include "reading/PaperReadingGoalTree.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QFontMetrics>
#include <QFont>
#include <QPainterPath>
#include <QDateTime>
#include <algorithm>
#include <cmath>

PaperReadingGoalTree::PaperReadingGoalTree(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingGoalTree")
    , updateBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
    updateInfo();
    setMinimumSize(800, 520);
}

void PaperReadingGoalTree::setupUI()
{
    auto* topBar = new QHBoxLayout;

    inputField_ = new QLineEdit;
    inputField_->setPlaceholderText(tr("Enter goal name..."));

    categoryCombo_ = new QComboBox;
    categoryCombo_->addItems({tr("Reading"), tr("Writing"), tr("Review"), tr("Research"), tr("Other")});

    updateBtn_ = new QPushButton(tr("Update"));
    clearBtn_ = new QPushButton(tr("Clear"));

    infoLabel_ = new QLabel;
    infoLabel_->setWordWrap(true);

    topBar->addWidget(inputField_, 3);
    topBar->addWidget(categoryCombo_, 1);
    topBar->addWidget(updateBtn_, 1);
    topBar->addWidget(clearBtn_, 1);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(topBar);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addStretch(1);

    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingGoalTree::onUpdate);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingGoalTree::onClear);
}

void PaperReadingGoalTree::addEntry(const GoalEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<GoalEntry> PaperReadingGoalTree::entries() const
{
    return entries_;
}

int PaperReadingGoalTree::achievedCount() const
{
    return std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const GoalEntry& e) { return e.achieved; });
}

qreal PaperReadingGoalTree::avgProgress() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.progress;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingGoalTree::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingGoalTree::onUpdate()
{
    const QString goalName = inputField_->text().trimmed();
    if (goalName.isEmpty())
        return;

    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    GoalEntry entry;
    entry.id = static_cast<int>(std::chrono::steady_clock::now().time_since_epoch().count()) & 0x7FFFFFFF;
    if (entry.id == 0) entry.id = 1;

    entry.goal = goalName;
    entry.category = categoryCombo_->currentText();
    entry.priority = QStringLiteral("Medium");

    entry.target = static_cast<qreal>(QRandomGenerator::global()->bounded(5, 26));
    entry.progress = static_cast<qreal>(QRandomGenerator::global()->bounded(0, static_cast<int>(entry.target) + 1));
    entry.papers = QRandomGenerator::global()->bounded(1, 51);
    entry.achieved = entry.progress >= entry.target;
    entry.color = palette[entries_.size() % palette.size()];

    addEntry(entry);
    emit goalUpdated(entry.id, entry.progress);

    inputField_->clear();
}

void PaperReadingGoalTree::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperReadingGoalTree::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int topBarHeight = 90;
    const int contentY = topBarHeight;
    const int w = width();
    const int h = height() - contentY;

    const QRect leftRect(10, contentY, w * 55 / 100, h - 10);
    const QRect rightTopRect(w * 56 / 100, contentY, w * 43 / 100, h * 50 / 100);
    const QRect rightBottomRect(w * 56 / 100, contentY + h * 52 / 100, w * 43 / 100, h * 46 / 100);

    drawGoalTree(p, leftRect);
    drawCategoryChart(p, rightTopRect);
    drawStats(p, rightBottomRect);
}

void PaperReadingGoalTree::drawGoalTree(QPainter& p, const QRect& rect)
{
    // Panel background
    QPainterPath bg;
    bg.addRoundedRect(rect, 12, 12);
    p.fillPath(bg, QColor(248, 250, 252));
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.drawPath(bg);

    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(16, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Goal Tree"));

    if (entries_.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(10);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect, Qt::AlignCenter, tr("No goals yet. Add one above."));
        return;
    }

    const int leftPad = 24;
    const int topPad = 42;
    const int rowHeight = 56;
    const int progressBarH = 10;
    const QFontMetrics fm(font());

    // Group by category
    QMap<QString, QList<int>> catIndices;
    for (int i = 0; i < entries_.size(); ++i)
        catIndices[entries_[i].category].append(i);

    int y = rect.y() + topPad;
    const int visibleHeight = rect.height() - topPad - 10;
    int drawn = 0;

    for (auto catIt = catIndices.constBegin(); catIt != catIndices.constEnd(); ++catIt) {
        if (y + rowHeight > rect.y() + rect.height() - 5)
            break;

        // Category header node
        QFont catFont = font();
        catFont.setBold(true);
        catFont.setPointSize(10);
        p.setFont(catFont);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + leftPad, y + 14, catIt.key());

        // Draw tree connector lines from category to children
        const int connectorX = rect.x() + leftPad + 6;
        const int childX = rect.x() + leftPad + 28;

        y += 22;

        for (int idx : catIt.value()) {
            if (y + rowHeight > rect.y() + rect.height() - 5)
                break;

            const GoalEntry& entry = entries_[idx];

            // Vertical connector
            p.setPen(QPen(QColor(203, 213, 225), 1));
            p.drawLine(connectorX, y - 8, connectorX, y + 6);
            // Horizontal connector
            p.drawLine(connectorX, y + 6, childX - 2, y + 6);

            // Goal node circle
            p.setBrush(entry.achieved ? QColor("#16a34a") : entry.color);
            p.setPen(Qt::NoPen);
            p.drawEllipse(childX - 5, y + 1, 10, 10);

            // Goal name
            QFont goalFont = font();
            goalFont.setPointSize(9);
            p.setFont(goalFont);
            p.setPen(QColor(30, 41, 59));
            const int textX = childX + 12;
            p.drawText(textX, y + 10, fm.elidedText(entry.goal, Qt::ElideRight, rect.width() - textX - leftPad - 40));

            // Paper count badge
            const QString papersStr = QStringLiteral("%1 papers").arg(entry.papers);
            const int papersW = fm.horizontalAdvance(papersStr) + 12;
            const int badgeX = rect.right() - leftPad - papersW;
            QPainterPath badge;
            badge.addRoundedRect(badgeX, y - 1, papersW, 16, 4, 4);
            p.fillPath(badge, QColor(241, 245, 249));
            p.drawPath(badge);
            p.setPen(QColor(100, 116, 139));
            p.drawText(badgeX + 6, y + 10, papersStr);

            y += 18;

            // Progress bar
            const int barW = rect.width() - childX - leftPad - 40;
            const int barX = childX + 12;
            const qreal pct = entry.target > 0.0 ? qMin(entry.progress / entry.target, 1.0) : 0.0;

            QPainterPath barBg;
            barBg.addRoundedRect(barX, y, barW, progressBarH, 4, 4);
            p.fillPath(barBg, QColor(226, 232, 240));

            if (pct > 0.0) {
                QPainterPath barFill;
                const int fillW = qMax(static_cast<int>(barW * pct), 1);
                barFill.addRoundedRect(barX, y, fillW, progressBarH, 4, 4);
                p.fillPath(barFill, entry.color);
            }

            // Percentage text
            p.setPen(QColor(100, 116, 139));
            QFont smallFont = font();
            smallFont.setPointSize(8);
            p.setFont(smallFont);
            p.drawText(barX + barW + 6, y + 9, QStringLiteral("%1%").arg(qRound(pct * 100)));

            y += progressBarH + 14;
            drawn++;
        }

        y += 6;
    }

    // Scroll indicator if some entries overflow
    if (y > rect.y() + rect.height() && drawn < entries_.size()) {
        QFont hintFont = font();
        hintFont.setPointSize(8);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect.adjusted(0, 0, -10, -4), Qt::AlignRight | Qt::AlignBottom,
                   tr("+ %1 more").arg(entries_.size() - drawn));
    }
}

void PaperReadingGoalTree::drawCategoryChart(QPainter& p, const QRect& rect)
{
    QPainterPath bg;
    bg.addRoundedRect(rect, 12, 12);
    p.fillPath(bg, QColor(248, 250, 252));
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.drawPath(bg);

    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(16, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Category Breakdown"));

    if (entries_.isEmpty()) {
        QFont hintFont = font();
        hintFont.setPointSize(9);
        p.setFont(hintFont);
        p.setPen(QColor(148, 163, 184));
        p.drawText(rect, Qt::AlignCenter, tr("No data"));
        return;
    }

    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    const QMap<QString, int> counts = categoryCounts();
    const int maxCount = *std::max_element(counts.constBegin(), counts.constEnd());
    const int barAreaX = rect.x() + 16;
    const int barAreaW = rect.width() - 100;
    const int labelX = rect.x() + 16;
    const int topPad = 40;
    const int rowH = 26;

    int i = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it, ++i) {
        const int y = rect.y() + topPad + i * rowH;
        if (y + rowH > rect.bottom() - 5)
            break;

        // Category label
        QFont catFont = font();
        catFont.setPointSize(9);
        p.setFont(catFont);
        p.setPen(QColor(71, 85, 105));
        p.drawText(labelX, y + 12, it.key());

        // Bar
        const int barX = labelX + 80;
        const int barH = 14;
        const qreal ratio = maxCount > 0 ? static_cast<qreal>(it.value()) / maxCount : 0.0;
        const int fillW = qMax(static_cast<int>((barAreaW - 80 - 50) * ratio), 0);

        QPainterPath barBg;
        barBg.addRoundedRect(barX, y + 2, barAreaW - 80 - 50, barH, 5, 5);
        p.fillPath(barBg, QColor(226, 232, 240));

        if (fillW > 0) {
            QPainterPath barFill;
            barFill.addRoundedRect(barX, y + 2, fillW, barH, 5, 5);
            p.fillPath(barFill, palette[i % palette.size()]);
        }

        // Count
        p.setPen(QColor(71, 85, 105));
        p.drawText(barX + barAreaW - 80 - 50 + 8, y + 13, QString::number(it.value()));
    }
}

void PaperReadingGoalTree::drawStats(QPainter& p, const QRect& rect)
{
    QPainterPath bg;
    bg.addRoundedRect(rect, 12, 12);
    p.fillPath(bg, QColor(248, 250, 252));
    p.setPen(QPen(QColor(226, 232, 240), 1));
    p.drawPath(bg);

    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.setPen(QColor(30, 41, 59));
    p.drawText(rect.adjusted(16, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Statistics"));

    QFont statsFont = font();
    statsFont.setPointSize(10);
    p.setFont(statsFont);

    const int leftX = rect.x() + 20;
    const int rightX = rect.x() + rect.width() / 2 + 20;
    const int y0 = rect.y() + 40;
    const int lineH = 24;

    // Total goals
    p.setPen(QColor(100, 116, 139));
    p.drawText(leftX, y0, tr("Total Goals:"));
    p.setPen(QColor(30, 41, 59));
    p.drawText(rightX, y0, QString::number(entries_.size()));

    // Achieved
    p.setPen(QColor(100, 116, 139));
    p.drawText(leftX, y0 + lineH, tr("Achieved:"));
    const int achieved = achievedCount();
    p.setPen(achieved > 0 ? QColor("#16a34a") : QColor(30, 41, 59));
    p.drawText(rightX, y0 + lineH, QString::number(achieved));

    // Avg progress
    p.setPen(QColor(100, 116, 139));
    p.drawText(leftX, y0 + lineH * 2, tr("Avg Progress:"));
    p.setPen(QColor("#3b82f6"));
    p.drawText(rightX, y0 + lineH * 2, QStringLiteral("%1%").arg(qRound(avgProgress() * 10) / 10.0, 0, 'f', 1));

    // Completion rate
    const qreal rate = entries_.isEmpty() ? 0.0
        : static_cast<qreal>(achieved) / entries_.size() * 100.0;
    p.setPen(QColor(100, 116, 139));
    p.drawText(leftX, y0 + lineH * 3, tr("Completion:"));
    p.setPen(rate >= 50.0 ? QColor("#16a34a") : QColor("#d97706"));
    p.drawText(rightX, y0 + lineH * 3, QStringLiteral("%1%").arg(rate, 0, 'f', 1));

    // Total papers
    int totalPapers = 0;
    for (const auto& e : entries_)
        totalPapers += e.papers;
    p.setPen(QColor(100, 116, 139));
    p.drawText(leftX, y0 + lineH * 4, tr("Total Papers:"));
    p.setPen(QColor("#7c3aed"));
    p.drawText(rightX, y0 + lineH * 4, QString::number(totalPapers));
}

void PaperReadingGoalTree::updateInfo()
{
    const int total = entries_.size();
    const int achieved = achievedCount();
    const qreal avg = avgProgress();

    infoLabel_->setText(
        tr("Goals: %1 | Achieved: %2 | Avg Progress: %3%")
            .arg(total)
            .arg(achieved)
            .arg(qRound(avg * 10) / 10.0, 0, 'f', 1));
}

void PaperReadingGoalTree::loadSettings()
{
    const int size = settings_.beginReadArray("entries");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GoalEntry e;
        e.id = settings_.value("id").toInt();
        e.goal = settings_.value("goal").toString();
        e.category = settings_.value("category").toString();
        e.priority = settings_.value("priority").toString();
        e.progress = settings_.value("progress").toDouble();
        e.target = settings_.value("target").toDouble();
        e.papers = settings_.value("papers").toInt();
        e.achieved = settings_.value("achieved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
}

void PaperReadingGoalTree::saveSettings()
{
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const GoalEntry& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("goal", e.goal);
        settings_.setValue("category", e.category);
        settings_.setValue("priority", e.priority);
        settings_.setValue("progress", e.progress);
        settings_.setValue("target", e.target);
        settings_.setValue("papers", e.papers);
        settings_.setValue("achieved", e.achieved);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}
