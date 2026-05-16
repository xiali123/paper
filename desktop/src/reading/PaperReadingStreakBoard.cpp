#include "reading/PaperReadingStreakBoard.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingStreakBoard::PaperReadingStreakBoard(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingStreakBoard")
{
    setupUI();
    loadSettings();
}

void PaperReadingStreakBoard::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    updateBtn_ = new QPushButton("Update");
    updateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(updateBtn_, &QPushButton::clicked, this, &PaperReadingStreakBoard::onUpdate);
    toolbar->addWidget(updateBtn_);
    toolbar->addWidget(new QLabel("Period:"));
    periodCombo_ = new QComboBox();
    periodCombo_->addItems({"Daily", "Weekly", "Monthly"});
    toolbar->addWidget(periodCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingStreakBoard::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter reader name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Track reading streaks");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingStreakBoard::addEntry(const StreakBoardEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit streakUpdated(entry.id, entry.currentStreak);
    update();
}

QList<StreakBoardEntry> PaperReadingStreakBoard::entries() const { return entries_; }

int PaperReadingStreakBoard::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperReadingStreakBoard::avgStreak() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.currentStreak;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingStreakBoard::periodCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.period]++;
    return counts;
}

void PaperReadingStreakBoard::onUpdate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList periods = {"daily", "weekly", "monthly"};
    QStringList badges = {"Bronze", "Silver", "Gold", "Diamond"};
    int pIdx = periodCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        StreakBoardEntry e;
        e.id = entries_.size() + 1;
        e.readerName = text.left(10) + " user" + QString::number(i);
        e.currentStreak = QRandomGenerator::global()->bounded(30);
        e.bestStreak = e.currentStreak + QRandomGenerator::global()->bounded(20);
        e.totalPapers = 10 + QRandomGenerator::global()->bounded(200);
        e.badge = badges[qMin(3, e.currentStreak / 7)];
        e.rank = i + 1;
        e.avgPerDay = 0.5 + QRandomGenerator::global()->bounded(50) / 10.0;
        e.period = periods[pIdx];
        e.active = e.currentStreak > 0;
        e.color = e.currentStreak >= 14 ? QColor(16,185,129) : (e.currentStreak >= 7 ? QColor(245,158,11) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingStreakBoard::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track reading streaks");
    update();
}

void PaperReadingStreakBoard::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading streaks");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Streak Board");
    int w = width(), h = height();
    drawStreakList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawPeriodChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingStreakBoard::drawStreakList(QPainter& p, const QRect& rect) {
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
                   "#" + QString::number(e.rank) + " " + e.readerName.left(12));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.badge + " | best:" + QString::number(e.bestStreak) + "d | " + QString::number(e.totalPapers) + " papers");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.currentStreak) + "d streak");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.avgPerDay, 'f', 1) + "/day | " + e.period);
    }
}

void PaperReadingStreakBoard::drawPeriodChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Periods");
    auto counts = periodCounts();
    QStringList periods = {"daily", "weekly", "monthly"};
    QString labels[] = {"Daily", "Weekly", "Monthly"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(periods[i]) ? counts[periods[i]] : 0;
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

void PaperReadingStreakBoard::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Readers", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(16,185,129)},
        {"Avg Streak", QString::number(avgStreak(), 'f', 0) + "d", QColor(245,158,11)},
        {"Periods", QString::number(periodCounts().size()), QColor(139,92,246)}
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

void PaperReadingStreakBoard::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track reading streaks"); return; }
    infoLabel_->setText(QString("%1 readers | %2 active | %3d avg streak")
        .arg(entries_.size()).arg(activeCount()).arg(avgStreak(), 0, 'f', 0));
}

void PaperReadingStreakBoard::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StreakBoardEntry e;
        e.id = settings_.value("id").toInt();
        e.readerName = settings_.value("readerName").toString();
        e.currentStreak = settings_.value("currentStreak").toInt();
        e.bestStreak = settings_.value("bestStreak").toInt();
        e.totalPapers = settings_.value("totalPapers").toInt();
        e.badge = settings_.value("badge").toString();
        e.rank = settings_.value("rank").toInt();
        e.avgPerDay = settings_.value("avgPerDay").toDouble();
        e.period = settings_.value("period").toString();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingStreakBoard::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("readerName", entries_[i].readerName);
        settings_.setValue("currentStreak", entries_[i].currentStreak);
        settings_.setValue("bestStreak", entries_[i].bestStreak);
        settings_.setValue("totalPapers", entries_[i].totalPapers);
        settings_.setValue("badge", entries_[i].badge);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("avgPerDay", entries_[i].avgPerDay);
        settings_.setValue("period", entries_[i].period);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
