#include "workspace/PaperSeminarScheduler2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>
#include <numeric>

PaperSeminarScheduler2::PaperSeminarScheduler2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SeminarScheduler2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList seminars = {"Machine Learning Trends", "Quantum Computing 101",
                                "Neural Architecture Search"};
        QStringList speakers = {"Dr. Smith", "Prof. Johnson", "Dr. Lee"};
        QStringList categories = {"AI", "Physics", "Biology", "Chemistry", "Mathematics"};
        QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6),
                           QColor(220,38,38), QColor(124,58,237)};
        for (int i = 0; i < 8; ++i) {
            SeminarScheduler2Entry e;
            e.id = i + 1;
            e.seminar = seminars[QRandomGenerator::global()->bounded(seminars.size())];
            e.speaker = speakers[QRandomGenerator::global()->bounded(speakers.size())];
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            e.duration = 0.5 + QRandomGenerator::global()->bounded(4);
            e.attendees = 5 + QRandomGenerator::global()->bounded(50);
            e.recorded = QRandomGenerator::global()->bounded(2) == 0;
            e.color = colors[QRandomGenerator::global()->bounded(5)];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperSeminarScheduler2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "AI", "Physics", "Biology", "Chemistry", "Mathematics"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search seminars...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    scheduleBtn_ = new QPushButton("Schedule");
    scheduleBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scheduleBtn_, &QPushButton::clicked, this, &PaperSeminarScheduler2::onSchedule);
    toolbar->addWidget(scheduleBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSeminarScheduler2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    infoLabel_ = new QLabel("Seminar Scheduler");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);
    setMinimumSize(720, 540);
}

void PaperSeminarScheduler2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No seminars scheduled");
        return;
    }

    int w = width(), h = height();
    int topHalf = static_cast<int>(h * 0.75);

    drawScheduleView(p, QRect(10, 10, static_cast<int>(w * 0.6), topHalf - 20));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6) + 10, 10,
                                w - static_cast<int>(w * 0.6) - 20, topHalf - 20));
    drawStats(p, QRect(10, topHalf, w - 20, h - topHalf - 10));
}

void PaperSeminarScheduler2::drawScheduleView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignTop,
               "Schedule Timeline");

    int maxShow = qMin(10, entries_.size());
    int headerH = 28;
    int cardH = qMin(52, (rect.height() - headerH - 10) / qMax(maxShow, 1));
    qreal maxDuration = 0;
    for (const auto& e : entries_) maxDuration = qMax(maxDuration, e.duration);

    for (int i = 0; i < maxShow; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + headerH + i * (cardH + 3);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), cardH, 6, 6);

        // Color accent bar on left
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 5, cardH, 2, 2);

        // Seminar name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 12, y + 2, rect.width() - 24, 18, Qt::AlignVCenter,
                   e.seminar);

        // Speaker badge
        p.setFont(QFont("Arial", 7));
        QRectF badgeRect(rect.x() + 12, y + 20, 80, 14);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        p.drawRoundedRect(badgeRect, 3, 3);
        p.setPen(Qt::white);
        p.drawText(badgeRect, Qt::AlignCenter, e.speaker);

        // Duration bar
        qreal barFrac = maxDuration > 0 ? e.duration / maxDuration : 0;
        int barMaxW = rect.width() / 3;
        int barW = static_cast<int>(barFrac * barMaxW);
        int barX = rect.x() + rect.width() / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, y + 4, barW, 10, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, y + 4, 60, 10, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(e.duration, 'f', 1) + "h");

        // Attendee count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, y + 18, barMaxW, 14, Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(e.attendees) + " attendees");

        // Recorded indicator
        if (e.recorded) {
            QRectF recBadge(rect.x() + rect.width() - 50, y + 4, 38, 14);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38, 180));
            p.drawRoundedRect(recBadge, 3, 3);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 7, QFont::Bold));
            p.drawText(recBadge, Qt::AlignCenter, "REC");
        }
    }
}

void PaperSeminarScheduler2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 22, Qt::AlignLeft | Qt::AlignTop,
               "Category Distribution");

    auto counts = categoryCounts();
    int total = entries_.size();
    if (total == 0) return;

    QStringList categories = {"AI", "Physics", "Biology", "Chemistry", "Mathematics"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};

    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + (rect.height() - 70) / 2;
    int radius = qMin(rect.width() / 2 - 10, (rect.height() - 70) / 2);
    if (radius < 20) radius = 20;

    qreal startAngle = 0.0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal span = (static_cast<qreal>(count) / total) * 360.0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(QRect(cx - radius, cy - radius, radius * 2, radius * 2),
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    // Legend
    int legendY = cy + radius + 12;
    int legendX = rect.x() + 4;
    p.setFont(QFont("Arial", 7));
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(legendX, legendY, 8, 8, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.drawText(legendX + 12, legendY + 8,
                   categories[i] + " (" + QString::number(count) + ")");
        legendX += 10 + p.fontMetrics().horizontalAdvance(
                            categories[i] + " (" + QString::number(count) + ")") + 12;
        if (legendX > rect.x() + rect.width() - 40) {
            legendX = rect.x() + 4;
            legendY += 14;
        }
    }
}

void PaperSeminarScheduler2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Seminars", QString::number(entries_.size()), QColor(59,130,246)},
        {"Recorded Count", QString::number(recordedCount()), QColor(22,163,106)},
        {"Avg Duration", QString::number(avgDuration(), 'f', 1) + "h", QColor(217,119,6)},
        {"Total Attendees", QString::number(std::accumulate(entries_.begin(), entries_.end(), 0,
            [](int sum, const SeminarScheduler2Entry& e) { return sum + e.attendees; })),
         QColor(124,58,237)}
    };

    int boxCount = stats.size();
    int gap = 10;
    int boxW = (rect.width() - gap * (boxCount - 1)) / boxCount;
    int boxH = rect.height() - 4;
    if (boxH < 30) boxH = 30;

    for (int i = 0; i < boxCount; ++i) {
        int x = rect.x() + i * (boxW + gap);
        int y = rect.y() + 2;

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 3, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 8, y + 6, boxW - 16, boxH / 2, Qt::AlignLeft | Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(x + 8, y + boxH / 2, boxW - 16, boxH / 2 - 4, Qt::AlignLeft | Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperSeminarScheduler2::addEntry(const SeminarScheduler2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit seminarScheduled(entry.id, entry.duration);
    update();
}

QList<SeminarScheduler2Entry> PaperSeminarScheduler2::entries() const {
    return entries_;
}

int PaperSeminarScheduler2::recordedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.recorded) ++c;
    return c;
}

qreal PaperSeminarScheduler2::avgDuration() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.duration;
    return total / entries_.size();
}

QMap<QString, int> PaperSeminarScheduler2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSeminarScheduler2::onSchedule() {
    QStringList seminars = {"Machine Learning Trends", "Quantum Computing 101",
                            "Neural Architecture Search"};
    QStringList speakers = {"Dr. Smith", "Prof. Johnson", "Dr. Lee"};
    QStringList categories = {"AI", "Physics", "Biology", "Chemistry", "Mathematics"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,106), QColor(217,119,6),
                       QColor(220,38,38), QColor(124,58,237)};

    SeminarScheduler2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.seminar = seminars[QRandomGenerator::global()->bounded(seminars.size())];
    e.speaker = speakers[QRandomGenerator::global()->bounded(speakers.size())];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.duration = 0.5 + QRandomGenerator::global()->bounded(4);
    e.attendees = 5 + QRandomGenerator::global()->bounded(50);
    e.recorded = QRandomGenerator::global()->bounded(2) == 0;
    e.color = colors[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperSeminarScheduler2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Seminar Scheduler");
    update();
}

void PaperSeminarScheduler2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Seminar Scheduler");
        return;
    }
    int totalAttendees = 0;
    for (const auto& e : entries_) totalAttendees += e.attendees;
    infoLabel_->setText(QString("%1 seminars | %2 recorded | %3h avg | %4 attendees")
        .arg(entries_.size())
        .arg(recordedCount())
        .arg(QString::number(avgDuration(), 'f', 1))
        .arg(totalAttendees));
}

void PaperSeminarScheduler2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SeminarScheduler2Entry e;
        e.id = settings_.value("id").toInt();
        e.seminar = settings_.value("seminar").toString();
        e.category = settings_.value("category").toString();
        e.speaker = settings_.value("speaker").toString();
        e.duration = settings_.value("duration").toReal();
        e.attendees = settings_.value("attendees").toInt();
        e.recorded = settings_.value("recorded").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSeminarScheduler2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("seminar", entries_[i].seminar);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("speaker", entries_[i].speaker);
        settings_.setValue("duration", entries_[i].duration);
        settings_.setValue("attendees", entries_[i].attendees);
        settings_.setValue("recorded", entries_[i].recorded);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
