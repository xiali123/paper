#include "workspace/PaperStandupTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperStandupTracker::PaperStandupTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StandupTracker")
{
    setupUI();
    loadSettings();
}

void PaperStandupTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    logBtn_ = new QPushButton("Log");
    logBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(logBtn_, &QPushButton::clicked, this, &PaperStandupTracker::onLog);
    toolbar->addWidget(logBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Dev", "Research", "Review", "Testing", "Infra"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStandupTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter team member name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Log standup entries");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(620, 520);
}

void PaperStandupTracker::addEntry(const StandupEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit standupLogged(entry.id, entry.member);
    update();
}

QList<StandupEntry> PaperStandupTracker::entries() const { return entries_; }

int PaperStandupTracker::blockedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.blocked) c++;
    return c;
}

QMap<QString, int> PaperStandupTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperStandupTracker::onLog() {
    QString member = inputField_->text().trimmed();
    if (member.isEmpty()) return;

    static const QVector<QColor> palette = {
        QColor(0x3b, 0x82, 0xf6), // #3b82f6
        QColor(0x16, 0xa3, 0x4a), // #16a34a
        QColor(0xd9, 0x77, 0x06), // #d97706
        QColor(0xdc, 0x26, 0x26), // #dc2626
        QColor(0x7c, 0x3a, 0xed)  // #7c3aed
    };

    static const QStringList categories = {"Dev", "Research", "Review", "Testing", "Infra"};

    static const QStringList yesterdayTasks = {
        "Reviewed PR #42 and left comments",
        "Implemented auth module endpoints",
        "Ran regression test suite (142 passed)",
        "Drafted methods section for paper",
        "Fixed memory leak in data pipeline",
        "Benchmarked query performance",
        "Wrote unit tests for parser",
        "Refactored notification service",
        "Analyzed experiment results from batch 7",
        "Set up CI pipeline for new service",
        "Updated dependency versions",
        "Resolved merge conflicts on release branch"
    };

    static const QStringList todayTasks = {
        "Continue API integration work",
        "Write integration tests for module",
        "Present findings at team sync",
        "Review paper draft from co-author",
        "Optimize database query layer",
        "Implement caching strategy",
        "Debug intermittent test failures",
        "Update documentation for v2 API",
        "Prototype new visualization widget",
        "Prepare demo for stakeholder meeting",
        "Profile application startup time",
        "Migrate config to new schema format"
    };

    static const QStringList blockers = {
        "", "", "", "", // 4/12 empty = ~33% chance of blocker
        "Waiting on API access credentials",
        "Build fails on CI with linking error",
        "Blocked by unresolved design question",
        "Need approval from security team",
        "External dependency not yet published",
        "Database migration pending DBA review",
        "Test environment intermittently unavailable",
        "Missing specification for edge cases"
    };

    static const QStringList statuses = {"on-track", "at-risk", "completed", "not-started"};

    int catIdx = categoryCombo_->currentIndex();
    QString category = (catIdx == 0)
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[catIdx - 1];

    QString yesterday = yesterdayTasks[QRandomGenerator::global()->bounded(yesterdayTasks.size())];
    QString today = todayTasks[QRandomGenerator::global()->bounded(todayTasks.size())];
    QString blocker = blockers[QRandomGenerator::global()->bounded(blockers.size())];
    bool blocked = !blocker.isEmpty();
    QString status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
    QColor color = palette[QRandomGenerator::global()->bounded(palette.size())];

    StandupEntry entry;
    entry.id = entries_.size() + 1;
    entry.member = member;
    entry.category = category;
    entry.status = status;
    entry.yesterday = yesterday;
    entry.today = today;
    entry.blocker = blocker;
    entry.blocked = blocked;
    entry.color = color;

    addEntry(entry);
    inputField_->clear();
}

void PaperStandupTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Log standup entries");
    update();
}

void PaperStandupTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Log standup entries");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Standup Tracker");

    int w = width(), h = height();
    drawStandupList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperStandupTracker::drawStandupList(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int rowH = 62;
    int y = rect.top();

    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 9));
    p.drawText(rect.left(), y, "Member / Yesterday / Today");
    y += 16;

    for (int i = entries_.size() - show; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        if (y + rowH > rect.bottom()) break;

        // Background card
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.left(), y, rect.width() - 4, rowH - 4, 6, 6);

        // Color accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(rect.left(), y, 4, rowH - 4, 2, 2);

        // Member name and category badge
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 10, QFont::Bold));
        p.drawText(rect.left() + 12, y + 14, e.member);

        // Category pill
        QFontMetrics fm(p.font());
        int memberW = fm.horizontalAdvance(e.member);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(140));
        p.drawRoundedRect(rect.left() + 14 + memberW, y + 4, fm.horizontalAdvance(e.category) + 10, 16, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.left() + 19 + memberW, y + 15, e.category);

        // Yesterday line
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        QString yText = "Y: " + e.yesterday;
        if (fm.horizontalAdvance(yText) > rect.width() - 24)
            yText = fm.elidedText(yText, Qt::ElideRight, rect.width() - 24);
        p.drawText(rect.left() + 12, y + 30, yText);

        // Today line
        QString tText = "T: " + e.today;
        if (fm.horizontalAdvance(tText) > rect.width() - 24)
            tText = fm.elidedText(tText, Qt::ElideRight, rect.width() - 24);
        p.drawText(rect.left() + 12, y + 43, tText);

        // Blocker indicator
        if (e.blocked) {
            p.setPen(QColor(220, 38, 38));
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(rect.left() + 12, y + 56, "BLOCKED: " + e.blocker.left(30));
        }

        y += rowH;
    }
}

void PaperStandupTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    if (counts.isEmpty()) return;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    static const QVector<QColor> chartColors = {
        QColor(0x3b, 0x82, 0xf6),
        QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06),
        QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.left(), rect.top(), "By Category");

    int barY = rect.top() + 20;
    int barH = 18;
    int idx = 0;
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        if (it.value() > maxCount) maxCount = it.value();

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (barY + barH > rect.bottom()) break;

        // Label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.left(), barY + 13, it.key());

        // Bar
        int labelW = 60;
        int barW = maxCount > 0 ? (rect.width() - labelW - 40) * it.value() / maxCount : 0;
        QColor barColor = chartColors[idx % chartColors.size()];
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left() + labelW, barY + 2, barW, barH - 4, 4, 4);

        // Count label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.left() + labelW + barW + 6, barY + 13, QString::number(it.value()));

        barY += barH + 4;
        ++idx;
    }
}

void PaperStandupTracker::drawStats(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(rect.left(), rect.top(), "Statistics");

    int y = rect.top() + 22;
    int blocked = blockedCount();
    int total = entries_.size();

    // Total entries
    p.setPen(QColor(71, 85, 105));
    p.setFont(QFont("Arial", 9));
    p.drawText(rect.left(), y, "Total entries:");
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.left() + 100, y, QString::number(total));
    y += 18;

    // Blocked count
    p.setPen(QColor(71, 85, 105));
    p.setFont(QFont("Arial", 9));
    p.drawText(rect.left(), y, "Blocked:");
    p.setPen(blocked > 0 ? QColor(220, 38, 38) : QColor(22, 163, 74));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.left() + 100, y, QString::number(blocked));
    y += 18;

    // Block rate
    double blockRate = total > 0 ? (double(blocked) / total) * 100.0 : 0.0;
    p.setPen(QColor(71, 85, 105));
    p.setFont(QFont("Arial", 9));
    p.drawText(rect.left(), y, "Block rate:");
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.left() + 100, y, QString::number(blockRate, 'f', 1) + "%");
    y += 18;

    // Categories tracked
    p.setPen(QColor(71, 85, 105));
    p.setFont(QFont("Arial", 9));
    p.drawText(rect.left(), y, "Categories:");
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.left() + 100, y, QString::number(categoryCounts().size()));
    y += 22;

    // Status breakdown
    QMap<QString, int> statusCounts;
    for (const auto& e : entries_) statusCounts[e.status]++;
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 9));
    p.drawText(rect.left(), y, "Status breakdown:");
    y += 16;

    static const QMap<QString, QColor> statusColors = {
        {"on-track",  QColor(22, 163, 74)},
        {"at-risk",   QColor(217, 119, 6)},
        {"completed", QColor(59, 130, 246)},
        {"not-started", QColor(148, 163, 184)}
    };

    for (auto it = statusCounts.constBegin(); it != statusCounts.constEnd(); ++it) {
        if (y > rect.bottom() - 10) break;
        QColor dotColor = statusColors.value(it.key(), QColor(148, 163, 184));
        p.setPen(Qt::NoPen);
        p.setBrush(dotColor);
        p.drawEllipse(rect.left() + 6, y - 7, 8, 8);
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.left() + 20, y, it.key() + ": " + QString::number(it.value()));
        y += 16;
    }
}

void PaperStandupTracker::updateInfo() {
    int total = entries_.size();
    int blocked = blockedCount();
    infoLabel_->setText(QString("Entries: %1 | Blocked: %2 | Categories: %3")
        .arg(total)
        .arg(blocked)
        .arg(categoryCounts().size()));
}

void PaperStandupTracker::loadSettings() {
    int count = settings_.beginReadArray("entries");
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        StandupEntry e;
        e.id = settings_.value("id").toInt();
        e.member = settings_.value("member").toString();
        e.category = settings_.value("category").toString();
        e.status = settings_.value("status").toString();
        e.yesterday = settings_.value("yesterday").toString();
        e.today = settings_.value("today").toString();
        e.blocker = settings_.value("blocker").toString();
        e.blocked = settings_.value("blocked").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
    update();
}

void PaperStandupTracker::saveSettings() {
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id", e.id);
        settings_.setValue("member", e.member);
        settings_.setValue("category", e.category);
        settings_.setValue("status", e.status);
        settings_.setValue("yesterday", e.yesterday);
        settings_.setValue("today", e.today);
        settings_.setValue("blocker", e.blocker);
        settings_.setValue("blocked", e.blocked);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}
