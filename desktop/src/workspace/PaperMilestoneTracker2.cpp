#include "workspace/PaperMilestoneTracker2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMilestoneTracker2::PaperMilestoneTracker2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MilestoneTracker2")
{
    setupUI();
    loadSettings();
}

void PaperMilestoneTracker2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperMilestoneTracker2::onTrack);
    toolbar->addWidget(trackBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Writing", "Review", "Publish"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter milestone...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMilestoneTracker2::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Track milestones");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperMilestoneTracker2::addEntry(const MilestoneTrackEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit milestoneHit(entry.id, entry.progress);
    update();
}

QList<MilestoneTrackEntry> PaperMilestoneTracker2::entries() const { return entries_; }

int PaperMilestoneTracker2::achievedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.achieved) c++;
    return c;
}

qreal PaperMilestoneTracker2::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperMilestoneTracker2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperMilestoneTracker2::onTrack() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Research", "Writing", "Review", "Publish"};
    QStringList deadlines = {"2026-06-01", "2026-07-15", "2026-08-30", "2026-09-15", "2026-10-01", "2026-12-31"};
    QList<QColor> colors = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626"), QColor("#7c3aed")};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        MilestoneTrackEntry e;
        e.id = entries_.size() + 1;
        e.milestone = text.left(8) + " ms" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.deadline = deadlines[QRandomGenerator::global()->bounded(deadlines.size())];
        e.progress = QRandomGenerator::global()->bounded(101);
        e.tasks = 1 + QRandomGenerator::global()->bounded(10);
        e.achieved = e.progress >= 100;
        e.color = colors[QRandomGenerator::global()->bounded(colors.size())];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperMilestoneTracker2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track milestones");
    update();
}

void PaperMilestoneTracker2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track milestones");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Milestone Tracker");
    int w = width(), h = height();
    drawTrackerView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperMilestoneTracker2::drawTrackerView(QPainter& p, const QRect& rect) {
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
                   e.milestone.left(14) + (e.achieved ? " [done]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.deadline + " | tasks:" + QString::number(e.tasks));
        int barX = rect.x() + rect.width() / 2;
        int barW = rect.width() / 2 - 40;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, y + 6, barW, 8, 4, 4);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 6, static_cast<int>(barW * e.progress / 100.0), 8, 4, 4);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, y + 14, QString::number(static_cast<int>(e.progress)) + "%");
    }
}

void PaperMilestoneTracker2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Research", "Writing", "Review", "Publish"};
    QColor colors[] = {QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626")};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperMilestoneTracker2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Milestones", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Achieved", QString::number(achievedCount()), QColor("#16a34a")},
        {"Avg Progress", QString::number(avgProgress(), 'f', 0) + "%", QColor("#d97706")},
        {"Categories", QString::number(categoryCounts().size()), QColor("#7c3aed")}
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

void PaperMilestoneTracker2::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track milestones"); return; }
    infoLabel_->setText(QString("%1 milestones | %2 achieved | %3% avg")
        .arg(entries_.size()).arg(achievedCount()).arg(avgProgress(), 0, 'f', 0));
}

void PaperMilestoneTracker2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MilestoneTrackEntry e;
        e.id = settings_.value("id").toInt();
        e.milestone = settings_.value("milestone").toString();
        e.category = settings_.value("category").toString();
        e.deadline = settings_.value("deadline").toString();
        e.progress = settings_.value("progress").toDouble();
        e.tasks = settings_.value("tasks").toInt();
        e.achieved = settings_.value("achieved").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperMilestoneTracker2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("milestone", entries_[i].milestone);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("deadline", entries_[i].deadline);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("tasks", entries_[i].tasks);
        settings_.setValue("achieved", entries_[i].achieved);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
