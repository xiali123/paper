#include "reading/PaperReadingAchievement.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingAchievement::PaperReadingAchievement(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingAchievement")
{
    setupUI();
    loadSettings();
}

void PaperReadingAchievement::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    unlockBtn_ = new QPushButton("Unlock");
    unlockBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(unlockBtn_, &QPushButton::clicked, this, &PaperReadingAchievement::onUnlock);
    toolbar->addWidget(unlockBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Reading", "Writing", "Review", "Research"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingAchievement::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter user name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading achievements");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingAchievement::addEntry(const AchievementEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit achievementUnlocked(entry.id, entry.points);
    update();
}

QList<AchievementEntry> PaperReadingAchievement::entries() const { return entries_; }

int PaperReadingAchievement::unlockedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.unlocked) c++;
    return c;
}

qreal PaperReadingAchievement::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingAchievement::tierCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.tier]++;
    return counts;
}

void PaperReadingAchievement::onUnlock() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList achievements = {"Paper Marathon", "Citation Hunter", "Deep Reader", "Speed Reviewer", "Knowledge Builder"};
    QStringList tiers = {"Bronze", "Silver", "Gold", "Platinum"};
    QStringList categories = {"reading", "writing", "review", "research"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        AchievementEntry e;
        e.id = entries_.size() + 1;
        e.userName = text.left(10);
        e.achievement = achievements[QRandomGenerator::global()->bounded(achievements.size())];
        e.tier = tiers[QRandomGenerator::global()->bounded(tiers.size())];
        e.points = 50 + QRandomGenerator::global()->bounded(500);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.papersRequired = 5 + QRandomGenerator::global()->bounded(50);
        e.papersDone = QRandomGenerator::global()->bounded(e.papersRequired + 10);
        e.progress = qMin(1.0, static_cast<qreal>(e.papersDone) / e.papersRequired);
        e.date = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(10));
        e.unlocked = e.progress >= 1.0;
        e.color = e.unlocked ? QColor(16,185,129) : (e.progress >= 0.5 ? QColor(245,158,11) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingAchievement::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading achievements");
    update();
}

void PaperReadingAchievement::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading achievements");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Achievements");
    int w = width(), h = height();
    drawAchievementList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTierChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingAchievement::drawAchievementList(QPainter& p, const QRect& rect) {
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
                   e.achievement.left(14) + (e.unlocked ? " [OK]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.tier + " | " + QString::number(e.papersDone) + "/" + QString::number(e.papersRequired));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.points) + " pts");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "% | " + e.date);
    }
}

void PaperReadingAchievement::drawTierChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Tiers");
    auto counts = tierCounts();
    QStringList tiers = {"Bronze", "Silver", "Gold", "Platinum"};
    QColor colors[] = {QColor(205,127,50), QColor(156,163,175), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(tiers[i]) ? counts[tiers[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, tiers[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingAchievement::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Achievements", QString::number(entries_.size()), QColor(59,130,246)},
        {"Unlocked", QString::number(unlockedCount()), QColor(16,185,129)},
        {"Avg Progress", QString::number(avgProgress() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Tiers", QString::number(tierCounts().size()), QColor(139,92,246)}
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

void PaperReadingAchievement::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading achievements"); return; }
    infoLabel_->setText(QString("%1 achievements | %2 unlocked | %3% progress")
        .arg(entries_.size()).arg(unlockedCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingAchievement::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        AchievementEntry e;
        e.id = settings_.value("id").toInt();
        e.userName = settings_.value("userName").toString();
        e.achievement = settings_.value("achievement").toString();
        e.tier = settings_.value("tier").toString();
        e.points = settings_.value("points").toInt();
        e.category = settings_.value("category").toString();
        e.papersRequired = settings_.value("papersRequired").toInt();
        e.papersDone = settings_.value("papersDone").toInt();
        e.progress = settings_.value("progress").toDouble();
        e.date = settings_.value("date").toString();
        e.unlocked = settings_.value("unlocked").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingAchievement::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("userName", entries_[i].userName);
        settings_.setValue("achievement", entries_[i].achievement);
        settings_.setValue("tier", entries_[i].tier);
        settings_.setValue("points", entries_[i].points);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("papersRequired", entries_[i].papersRequired);
        settings_.setValue("papersDone", entries_[i].papersDone);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("unlocked", entries_[i].unlocked);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
