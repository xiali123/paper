#include "reading/PaperReadingSpeedTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperReadingSpeedTracker::PaperReadingSpeedTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingSpeedTracker")
{
    setupUI();
    loadSettings();
}

void PaperReadingSpeedTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Log Session");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperReadingSpeedTracker::onAdd);
    toolbar->addWidget(addBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Fast", "Normal", "Slow"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSpeedTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track reading speed");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 480);
}

void PaperReadingSpeedTracker::addEntry(const SpeedEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit speedRecorded(entry.id, entry.pagesPerHour);
    update();
}

QList<SpeedEntry> PaperReadingSpeedTracker::entries() const { return entries_; }

qreal PaperReadingSpeedTracker::avgPagesPerHour() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.pagesPerHour;
    return sum / entries_.size();
}

int PaperReadingSpeedTracker::totalPagesRead() const {
    int t = 0;
    for (const auto& e : entries_) t += e.pagesRead;
    return t;
}

int PaperReadingSpeedTracker::totalMinutes() const {
    int t = 0;
    for (const auto& e : entries_) t += e.minutesSpent;
    return t;
}

void PaperReadingSpeedTracker::onAdd() {
    bool ok;
    QString title = QInputDialog::getText(this, "Log Session", "Paper:", QLineEdit::Normal, "", &ok);
    if (!ok || title.isEmpty()) return;

    QStringList diffs = {"easy", "medium", "hard"};
    QString diff = QInputDialog::getItem(this, "Log Session", "Difficulty:", diffs, 1, false, &ok);
    if (!ok) return;

    SpeedEntry e;
    e.id = entries_.size() + 1;
    e.paperTitle = title;
    e.pagesRead = 3 + QRandomGenerator::global()->bounded(25);
    e.minutesSpent = 10 + QRandomGenerator::global()->bounded(120);
    e.pagesPerHour = static_cast<qreal>(e.pagesRead) / e.minutesSpent * 60;
    e.difficulty = diff;
    e.comprehension = 40 + QRandomGenerator::global()->bounded(60);
    e.session = "Session " + QString::number(e.id);

    QColor diffColors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};
    int dIdx = diffs.indexOf(diff);
    e.color = diffColors[qBound(0, dIdx, 2)];
    addEntry(e);
}

void PaperReadingSpeedTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track reading speed");
    update();
}

void PaperReadingSpeedTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track reading speed");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Speed Tracker");

    int w = width(), h = height();
    drawSpeedList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawSpeedChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingSpeedTracker::drawSpeedList(QPainter& p, const QRect& rect) {
    int filterIdx = filterCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];
        if (filterIdx == 1 && e.pagesPerHour < 15) continue;
        if (filterIdx == 2 && (e.pagesPerHour < 8 || e.pagesPerHour >= 15)) continue;
        if (filterIdx == 3 && e.pagesPerHour >= 8) continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.paperTitle.left(18));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.difficulty + " | " + QString::number(e.pagesRead) + "pg " + QString::number(e.minutesSpent) + "min");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.pagesPerHour, 'f', 1) + " pg/hr");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.comprehension) + "% comp");
        show++;
    }
}

void PaperReadingSpeedTracker::drawSpeedChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Speed Over Time");

    int show = qMin(12, entries_.size());
    if (show < 2) return;
    qreal maxSpeed = 1;
    for (int i = 0; i < show; ++i) maxSpeed = qMax(maxSpeed, entries_[i].pagesPerHour);

    int chartH = rect.height() - 25;
    int chartW = rect.width() - 10;
    int startX = rect.x() + 5;
    int startY = rect.y() + 20;

    qreal stepX = static_cast<qreal>(chartW) / (show - 1);
    QPolygonF line;
    for (int i = 0; i < show; ++i) {
        qreal x = startX + i * stepX;
        qreal y = startY + chartH - (entries_[i].pagesPerHour / maxSpeed) * chartH;
        line << QPointF(x, y);

        p.setPen(Qt::NoPen);
        p.setBrush(entries_[i].color);
        p.drawEllipse(QPointF(x, y), 3, 3);
    }

    p.setPen(QPen(QColor(59,130,246), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPolyline(line);
}

void PaperReadingSpeedTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Sessions", QString::number(entries_.size()), QColor(59,130,246)},
        {"Pages", QString::number(totalPagesRead()), QColor(16,185,129)},
        {"Avg Speed", QString::number(avgPagesPerHour(), 'f', 1) + " pg/hr", QColor(245,158,11)},
        {"Total Time", QString::number(totalMinutes() / 60) + "h " + QString::number(totalMinutes() % 60) + "m", QColor(139,92,246)}
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

void PaperReadingSpeedTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track reading speed"); return; }
    infoLabel_->setText(QString("%1 sessions | %2 pages | %3 pg/hr avg")
        .arg(entries_.size()).arg(totalPagesRead()).arg(avgPagesPerHour(), 0, 'f', 1));
}

void PaperReadingSpeedTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SpeedEntry e;
        e.id = settings_.value("id").toInt();
        e.paperTitle = settings_.value("paperTitle").toString();
        e.pagesRead = settings_.value("pagesRead").toInt();
        e.minutesSpent = settings_.value("minutesSpent").toInt();
        e.pagesPerHour = settings_.value("pagesPerHour").toDouble();
        e.difficulty = settings_.value("difficulty").toString();
        e.comprehension = settings_.value("comprehension").toInt();
        e.session = settings_.value("session").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingSpeedTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paperTitle", entries_[i].paperTitle);
        settings_.setValue("pagesRead", entries_[i].pagesRead);
        settings_.setValue("minutesSpent", entries_[i].minutesSpent);
        settings_.setValue("pagesPerHour", entries_[i].pagesPerHour);
        settings_.setValue("difficulty", entries_[i].difficulty);
        settings_.setValue("comprehension", entries_[i].comprehension);
        settings_.setValue("session", entries_[i].session);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
