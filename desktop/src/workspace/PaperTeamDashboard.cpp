#include "workspace/PaperTeamDashboard.hpp"
#include <QPainter>
#include <QPainterPath>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <algorithm>
#include <numeric>

PaperTeamDashboard::PaperTeamDashboard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PaperTeamDashboard")
{
    setupUI();
    loadSettings();
}

void PaperTeamDashboard::setupUI()
{
    setMinimumSize(620, 520);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    // Toolbar
    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(8);

    refreshBtn_ = new QPushButton(tr("Refresh"));
    refreshBtn_->setStyleSheet(
        "QPushButton { background-color: #3b82f6; color: white; border: none; "
        "border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #2563eb; }"
        "QPushButton:pressed { background-color: #1d4ed8; }");
    connect(refreshBtn_, &QPushButton::clicked, this, &PaperTeamDashboard::onRefresh);
    toolbar->addWidget(refreshBtn_);

    clearBtn_ = new QPushButton(tr("Clear"));
    clearBtn_->setStyleSheet(
        "QPushButton { background-color: #dc2626; color: white; border: none; "
        "border-radius: 6px; padding: 8px 18px; font-weight: bold; font-size: 13px; }"
        "QPushButton:hover { background-color: #b91c1c; }"
        "QPushButton:pressed { background-color: #991b1b; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTeamDashboard::onClear);
    toolbar->addWidget(clearBtn_);

    categoryCombo_ = new QComboBox;
    categoryCombo_->addItems({tr("All"), tr("Research"), tr("Writing"),
                              tr("Review"), tr("Analysis"), tr("Administration")});
    categoryCombo_->setStyleSheet(
        "QComboBox { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; "
        "min-width: 130px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit;
    inputField_->setPlaceholderText(tr("Filter member name..."));
    inputField_->setStyleSheet(
        "QLineEdit { border: 1px solid #d1d5db; border-radius: 6px; padding: 6px 12px; }");
    toolbar->addWidget(inputField_, 1);

    infoLabel_ = new QLabel(tr("Team Dashboard"));
    infoLabel_->setStyleSheet("color: #6b7280; font-size: 12px;");
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
}

void PaperTeamDashboard::addEntry(const TeamDashboardEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    update();
    saveSettings();
    emit memberUpdated(entry.id, entry.productivity);
}

QList<TeamDashboardEntry> PaperTeamDashboard::entries() const
{
    return entries_;
}

int PaperTeamDashboard::activeCount() const
{
    return static_cast<int>(std::count_if(entries_.constBegin(), entries_.constEnd(),
        [](const TeamDashboardEntry& e) { return e.active; }));
}

qreal PaperTeamDashboard::avgProductivity() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal total = std::accumulate(entries_.constBegin(), entries_.constEnd(), 0.0,
        [](qreal sum, const TeamDashboardEntry& e) { return sum + e.productivity; });
    return total / entries_.size();
}

QMap<QString, int> PaperTeamDashboard::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperTeamDashboard::onRefresh()
{
    entries_.clear();

    const QStringList members = {"Alice", "Bob", "Carol", "Dave",
                                 "Eve", "Frank", "Grace", "Hank"};
    const QStringList categories = {"Research", "Writing", "Review",
                                    "Analysis", "Administration"};
    const QStringList roles = {"Lead", "Contributor", "Reviewer",
                               "Analyst", "Coordinator"};
    const QStringList palette = {"#3b82f6", "#16a34a", "#d97706",
                                 "#dc2626", "#7c3aed"};

    for (int i = 0; i < 8; ++i) {
        TeamDashboardEntry e;
        e.id = i + 1;
        e.member = members.at(i);
        e.category = categories.at(i % categories.size());
        e.role = roles.at(i % roles.size());
        e.productivity = 30.0 + QRandomGenerator::global()->generateDouble() * 70.0;
        e.tasks = 2 + QRandomGenerator::global()->bounded(15);
        e.active = (i % 3) != 2; // 6 of 8 active
        e.color = QColor(palette.at(i % palette.size()));
        entries_.append(e);
        emit memberUpdated(e.id, e.productivity);
    }

    updateInfo();
    update();
    saveSettings();
}

void PaperTeamDashboard::onClear()
{
    entries_.clear();
    updateInfo();
    update();
    saveSettings();
}

void PaperTeamDashboard::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    const int chartHeight = 140;
    const QRect dashboardRect(0, 50, width(), height() - 50 - chartHeight);
    const QRect chartRect(0, height() - chartHeight, width() / 2, chartHeight);
    const QRect statsRect(width() / 2, height() - chartHeight, width() / 2, chartHeight);

    drawDashboardView(p, dashboardRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperTeamDashboard::drawDashboardView(QPainter& p, const QRect& rect)
{
    if (entries_.isEmpty()) {
        p.setFont(QFont("Sans", 11));
        p.setPen(QColor("#9ca3af"));
        p.drawText(rect, Qt::AlignCenter, tr("No team entries. Click Refresh to populate."));
        return;
    }

    int y = rect.top() + 4;

    // Title
    p.setFont(QFont("Sans", 12, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y + 14, tr("Team Members"));
    y += 28;

    // Header row
    p.setFont(QFont("Sans", 9, QFont::Bold));
    p.setPen(QColor("#6b7280"));
    p.drawText(46, y + 10, tr("Member"));
    p.drawText(150, y + 10, tr("Category"));
    p.drawText(270, y + 10, tr("Role"));
    p.drawText(360, y + 10, tr("Productivity"));
    p.drawText(490, y + 10, tr("Tasks"));
    p.drawText(550, y + 10, tr("Status"));
    y += 20;

    // Separator
    p.setPen(QPen(QColor("#e5e7eb"), 1));
    p.drawLine(12, y, rect.right() - 12, y);
    y += 6;

    p.setFont(QFont("Sans", 10));
    for (const auto& e : entries_) {
        if (y + 40 > rect.bottom()) break;

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f9fafb"));
        p.drawRoundedRect(10, y - 2, rect.width() - 20, 38, 6, 6);

        // Color indicator
        p.setBrush(e.color);
        p.drawRoundedRect(14, y + 4, 6, 26, 3, 3);

        // Active dot
        p.setBrush(e.active ? QColor("#16a34a") : QColor("#9ca3af"));
        p.drawEllipse(26, y + 12, 8, 8);

        // Member name
        p.setPen(QColor("#1f2937"));
        p.setFont(QFont("Sans", 10));
        p.drawText(46, y + 20, e.member);

        // Category
        p.setPen(QColor("#6b7280"));
        p.drawText(150, y + 20, e.category);

        // Role
        p.drawText(270, y + 20, e.role);

        // Productivity bar
        int barMaxWidth = 100;
        int barW = static_cast<int>((e.productivity / 100.0) * barMaxWidth);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(360, y + 10, barW, 12, 4, 4);
        p.setPen(QColor("#374151"));
        p.setFont(QFont("Sans", 9));
        p.drawText(360 + barW + 4, y + 20, QString("%1%").arg(e.productivity, 0, 'f', 0));

        // Task count
        p.setPen(QColor("#374151"));
        p.setFont(QFont("Sans", 10));
        p.drawText(490, y + 20, QString::number(e.tasks));

        // Status
        p.setPen(e.active ? QColor("#16a34a") : QColor("#9ca3af"));
        p.setFont(QFont("Sans", 9));
        p.drawText(550, y + 20, e.active ? tr("Active") : tr("Inactive"));

        y += 44;
    }
}

void PaperTeamDashboard::drawCategoryChart(QPainter& p, const QRect& rect)
{
    if (entries_.isEmpty()) return;

    QMap<QString, int> counts = categoryCounts();
    const QStringList palette = {"#3b82f6", "#16a34a", "#d97706",
                                 "#dc2626", "#7c3aed"};

    int y = rect.top() + 16;

    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(12, y, tr("By Category"));
    y += 18;

    int maxVal = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxVal = std::max(maxVal, it.value());
    if (maxVal == 0) maxVal = 1;

    int idx = 0;
    p.setFont(QFont("Sans", 9));
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        QColor barColor(palette.at(idx % palette.size()));

        // Label
        p.setPen(QColor("#374151"));
        p.drawText(12, y + 10, it.key());

        // Bar
        int bw = static_cast<int>((static_cast<double>(it.value()) / maxVal) * 160);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(110, y, bw, 16, 4, 4);

        // Count
        p.setPen(QColor("#374151"));
        p.drawText(110 + bw + 6, y + 12, QString::number(it.value()));

        y += 22;
        ++idx;
    }
}

void PaperTeamDashboard::drawStats(QPainter& p, const QRect& rect)
{
    if (entries_.isEmpty()) return;

    int x = rect.left() + 16;
    int y = rect.top() + 16;

    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.setPen(QColor("#1f2937"));
    p.drawText(x, y, tr("Statistics"));
    y += 20;

    const int active = activeCount();
    const qreal avgProd = avgProductivity();
    const int totalTasks = std::accumulate(entries_.constBegin(), entries_.constEnd(), 0,
        [](int sum, const TeamDashboardEntry& e) { return sum + e.tasks; });

    QSet<QString> cats;
    for (const auto& e : entries_) cats.insert(e.category);

    p.setFont(QFont("Sans", 9));
    p.setPen(QColor("#6b7280"));
    p.drawText(x, y, tr("Total members: %1").arg(entries_.size()));
    y += 16;
    p.drawText(x, y, tr("Active: %1 / %2").arg(active).arg(entries_.size()));
    y += 16;
    p.drawText(x, y, tr("Avg productivity: %1%").arg(avgProd, 0, 'f', 1));
    y += 16;
    p.drawText(x, y, tr("Total tasks: %1").arg(totalTasks));
    y += 16;
    p.drawText(x, y, tr("Categories: %1").arg(cats.size()));

    // Mini active/inactive pie indicator
    if (!entries_.isEmpty()) {
        const int pieSize = 40;
        const int pieX = rect.right() - pieSize - 20;
        const int pieY = rect.top() + 20;
        const qreal activeRatio = static_cast<qreal>(active) / entries_.size();

        // Active slice (green)
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#16a34a"));
        p.drawPie(pieX, pieY, pieSize, pieSize,
                  90 * 16, static_cast<int>(-activeRatio * 360 * 16));

        // Inactive slice (gray)
        p.setBrush(QColor("#9ca3af"));
        p.drawPie(pieX, pieY, pieSize, pieSize,
                  static_cast<int>(90 * 16 - activeRatio * 360 * 16),
                  static_cast<int>(-(1.0 - activeRatio) * 360 * 16));

        p.setPen(QColor("#374151"));
        p.setFont(QFont("Sans", 8));
        p.drawText(pieX, pieY + pieSize + 12, tr("Active"));
    }
}

void PaperTeamDashboard::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("Team Dashboard"));
        return;
    }

    const int active = activeCount();
    const qreal avgProd = avgProductivity();

    infoLabel_->setText(tr("%1 members | %2 active | %3 avg prod")
        .arg(entries_.size())
        .arg(active)
        .arg(avgProd, 0, 'f', 1));
}

void PaperTeamDashboard::loadSettings()
{
    const int size = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TeamDashboardEntry e;
        e.id = settings_.value("id").toInt();
        e.member = settings_.value("member").toString();
        e.category = settings_.value("category").toString();
        e.role = settings_.value("role").toString();
        e.productivity = settings_.value("productivity").toReal();
        e.tasks = settings_.value("tasks").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    if (entries_.isEmpty()) {
        onRefresh();
    } else {
        updateInfo();
        update();
    }
}

void PaperTeamDashboard::saveSettings()
{
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_.at(i);
        settings_.setValue("id", e.id);
        settings_.setValue("member", e.member);
        settings_.setValue("category", e.category);
        settings_.setValue("role", e.role);
        settings_.setValue("productivity", e.productivity);
        settings_.setValue("tasks", e.tasks);
        settings_.setValue("active", e.active);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
}
