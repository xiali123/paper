#include "reading/PaperReadingGamification.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingGamification::PaperReadingGamification(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingGamification")
{
    setupUI();
    loadSettings();
}

void PaperReadingGamification::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingGamification::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Weekly", "Monthly", "All Time"});
    toolbar->addWidget(periodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingGamification::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter user name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Reading gamification");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingGamification::addEntry(const GamificationEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit gamificationUpdated(entry.id, entry.xpPoints);
    update();
}

QList<GamificationEntry> PaperReadingGamification::entries() const { return entries_; }

qreal PaperReadingGamification::avgXp() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.xpPoints;
    return sum / entries_.size();
}

int PaperReadingGamification::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

QMap<QString, int> PaperReadingGamification::rankCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.rank]++;
    return counts;
}

void PaperReadingGamification::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList ranks = {"Novice", "Reader", "Scholar", "Expert", "Master"};
    QStringList achievements = {"First Paper", "Speed Reader", "Deep Diver", "Consistent", "Explorer"};
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        GamificationEntry e;
        e.id = entries_.size() + 1;
        e.userName = text.left(10) + " player" + QString::number(i);
        e.xpPoints = 100 + QRandomGenerator::global()->bounded(5000);
        e.level = 1 + e.xpPoints / 500;
        e.rank = ranks[qMin(4, e.level - 1)];
        e.challengesCompleted = QRandomGenerator::global()->bounded(30);
        e.completionRate = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.streakDays = QRandomGenerator::global()->bounded(60);
        e.achievement = achievements[QRandomGenerator::global()->bounded(achievements.size())];
        e.active = e.streakDays > 0;
        e.color = e.level >= 4 ? QColor(139,92,246) : (e.level >= 2 ? QColor(16,185,129) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingGamification::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Reading gamification");
    update();
}

void PaperReadingGamification::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Reading gamification");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Gamification");
    int w = width(), h = height();
    drawLeaderboard(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawRankChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingGamification::drawLeaderboard(QPainter& p, const QRect& rect) {
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
                   "#" + QString::number(i + 1) + " " + e.userName.left(12));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.rank + " | Lv" + QString::number(e.level) + " | " + QString::number(e.streakDays) + "d");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.xpPoints) + " XP");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.challengesCompleted) + " challenges | " + e.achievement.left(10));
    }
}

void PaperReadingGamification::drawRankChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Ranks");
    auto counts = rankCounts();
    QStringList ranks = {"Novice", "Reader", "Scholar", "Expert", "Master"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246), QColor(239,68,68)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(ranks[i]) ? counts[ranks[i]] : 0;
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

void PaperReadingGamification::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Players", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(16,185,129)},
        {"Avg XP", QString::number(avgXp(), 'f', 0), QColor(245,158,11)},
        {"Ranks", QString::number(rankCounts().size()), QColor(139,92,246)}
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

void PaperReadingGamification::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Reading gamification"); return; }
    infoLabel_->setText(QString("%1 players | %2 active | %3 avg XP")
        .arg(entries_.size()).arg(activeCount()).arg(avgXp(), 0, 'f', 0));
}

void PaperReadingGamification::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GamificationEntry e;
        e.id = settings_.value("id").toInt();
        e.userName = settings_.value("userName").toString();
        e.xpPoints = settings_.value("xpPoints").toInt();
        e.level = settings_.value("level").toInt();
        e.rank = settings_.value("rank").toString();
        e.challengesCompleted = settings_.value("challengesCompleted").toInt();
        e.completionRate = settings_.value("completionRate").toDouble();
        e.streakDays = settings_.value("streakDays").toInt();
        e.achievement = settings_.value("achievement").toString();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingGamification::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("userName", entries_[i].userName);
        settings_.setValue("xpPoints", entries_[i].xpPoints);
        settings_.setValue("level", entries_[i].level);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("challengesCompleted", entries_[i].challengesCompleted);
        settings_.setValue("completionRate", entries_[i].completionRate);
        settings_.setValue("streakDays", entries_[i].streakDays);
        settings_.setValue("achievement", entries_[i].achievement);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
