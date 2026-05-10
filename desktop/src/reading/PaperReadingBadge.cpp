#include "reading/PaperReadingBadge.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingBadge::PaperReadingBadge(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingBadge")
{
    setupUI();
    loadSettings();
}

void PaperReadingBadge::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    earnBtn_ = new QPushButton("Earn");
    earnBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(earnBtn_, &QPushButton::clicked, this, &PaperReadingBadge::onEarn);
    toolbar->addWidget(earnBtn_);

    toolbar->addWidget(new QLabel("Tier:"));
    tierCombo_ = new QComboBox();
    tierCombo_->addItems({"Bronze", "Silver", "Gold", "Platinum"});
    toolbar->addWidget(tierCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingBadge::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter badge name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Earn reading badges");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingBadge::addEntry(const BadgeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit badgeEarned(entry.id, entry.progress);
    update();
}

QList<BadgeEntry> PaperReadingBadge::entries() const { return entries_; }

int PaperReadingBadge::earnedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.earned) c++;
    return c;
}

qreal PaperReadingBadge::avgProgress() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.progress;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingBadge::tierCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.tier]++;
    return counts;
}

void PaperReadingBadge::onEarn() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList tiers = {"bronze", "silver", "gold", "platinum"};
    QStringList icons = {"star", "shield", "crown", "diamond"};
    QStringList types = {"speed", "volume", "streak", "topic"};

    int tIdx = tierCombo_->currentIndex();
    BadgeEntry e;
    e.id = entries_.size() + 1;
    e.badgeName = text.left(14);
    e.tier = tiers[tIdx];
    e.badgeType = types[QRandomGenerator::global()->bounded(types.size())];
    e.papersRequired = (tIdx + 1) * 10;
    e.papersRead = QRandomGenerator::global()->bounded(e.papersRequired + 5);
    e.progress = qMin(1.0, static_cast<qreal>(e.papersRead) / e.papersRequired);
    e.icon = icons[tIdx];
    e.earned = e.progress >= 1.0;
    e.color = e.earned ? QColor(16,185,129) : (e.progress >= 0.5 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperReadingBadge::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Earn reading badges");
    update();
}

void PaperReadingBadge::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Earn reading badges");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Badges");

    int w = width(), h = height();
    drawBadgeList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTierChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingBadge::drawBadgeList(QPainter& p, const QRect& rect) {
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

        p.setBrush(e.color.lighter(140));
        p.drawEllipse(rect.x() + rect.width() - 24, y + 4, 20, 20);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 24, y + 4, 20, 20, Qt::AlignCenter,
                   e.icon.left(1).toUpper());

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.badgeName.left(14) + (e.earned ? " [OK]" : ""));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.tier + " | " + e.badgeType);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 40, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.papersRead) + "/" + QString::number(e.papersRequired));
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 40, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.progress * 100, 'f', 0) + "% complete");
    }
}

void PaperReadingBadge::drawTierChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Tiers");

    auto counts = tierCounts();
    QStringList tiers = {"bronze", "silver", "gold", "platinum"};
    QString labels[] = {"Bronze", "Silver", "Gold", "Platinum"};
    QColor colors[] = {QColor(205,127,50), QColor(192,192,192), QColor(255,215,0), QColor(169,169,169)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(tiers[i]) ? counts[tiers[i]] : 0;
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

void PaperReadingBadge::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Badges", QString::number(entries_.size()), QColor(59,130,246)},
        {"Earned", QString::number(earnedCount()), QColor(16,185,129)},
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

void PaperReadingBadge::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Earn reading badges"); return; }
    infoLabel_->setText(QString("%1 badges | %2 earned | %3% avg")
        .arg(entries_.size()).arg(earnedCount()).arg(avgProgress() * 100, 0, 'f', 0));
}

void PaperReadingBadge::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BadgeEntry e;
        e.id = settings_.value("id").toInt();
        e.badgeName = settings_.value("badgeName").toString();
        e.badgeType = settings_.value("badgeType").toString();
        e.papersRequired = settings_.value("papersRequired").toInt();
        e.papersRead = settings_.value("papersRead").toInt();
        e.progress = settings_.value("progress").toDouble();
        e.tier = settings_.value("tier").toString();
        e.icon = settings_.value("icon").toString();
        e.earned = settings_.value("earned").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingBadge::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("badgeName", entries_[i].badgeName);
        settings_.setValue("badgeType", entries_[i].badgeType);
        settings_.setValue("papersRequired", entries_[i].papersRequired);
        settings_.setValue("papersRead", entries_[i].papersRead);
        settings_.setValue("progress", entries_[i].progress);
        settings_.setValue("tier", entries_[i].tier);
        settings_.setValue("icon", entries_[i].icon);
        settings_.setValue("earned", entries_[i].earned);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
