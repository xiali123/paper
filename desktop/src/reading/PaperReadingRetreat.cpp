#include "reading/PaperReadingRetreat.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QFont>
#include <QtMath>
#include <QCoreApplication>

static const QVector<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

PaperReadingRetreat::PaperReadingRetreat(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingRetreat")
    , startBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingRetreat::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);

    startBtn_ = new QPushButton(tr("Start Retreat"), this);
    startBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    toolbar->addWidget(startBtn_);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Machine Learning"));
    categoryCombo_->addItem(tr("NLP"));
    categoryCombo_->addItem(tr("Computer Vision"));
    categoryCombo_->addItem(tr("Systems"));
    categoryCombo_->addItem(tr("Theory"));
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter retreat topic..."));
    inputField_->setMinimumWidth(220);
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:pressed{background:#991b1b;}");
    toolbar->addWidget(clearBtn_);

    root->addLayout(toolbar);

    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("font-size:13px; color:#6b7280;");
    root->addWidget(infoLabel_);

    root->addStretch(1);

    setMinimumSize(640, 420);

    connect(startBtn_, &QPushButton::clicked, this, &PaperReadingRetreat::onStart);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingRetreat::onClear);
}

void PaperReadingRetreat::addEntry(const RetreatEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<RetreatEntry> PaperReadingRetreat::entries() const
{
    return entries_;
}

int PaperReadingRetreat::completedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.completed)
            ++count;
    }
    return count;
}

qreal PaperReadingRetreat::avgDepth() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.depth;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingRetreat::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperReadingRetreat::onStart()
{
    auto* rng = QRandomGenerator::global();

    const QStringList focuses = {
        "deep-reading", "note-taking", "review", "synthesis", "critique"
    };

    const int n = rng->bounded(3, 7);

    for (int i = 0; i < n; ++i) {
        RetreatEntry entry;
        entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000) + i;
        entry.topic = inputField_->text().trimmed().isEmpty()
            ? QStringLiteral("Retreat-%1").arg(entries_.size() + 1)
            : inputField_->text().trimmed();
        entry.category = categoryCombo_->currentText();
        entry.focus = focuses.at(rng->bounded(static_cast<int>(focuses.size())));
        entry.depth = rng->bounded(30, 100) / 10.0;
        entry.hours = rng->bounded(1, 9);
        entry.completed = rng->bounded(0, 2) == 1;
        entry.color = kPalette.at(rng->bounded(static_cast<int>(kPalette.size())));
        entries_.append(entry);
    }

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    for (const auto& e : entries_.mid(entries_.size() - n))
        emit retreatDone(e.id, e.depth);
}

void PaperReadingRetreat::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingRetreat::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    p.fillRect(rect(), QColor("#f8fafc"));

    const int controlH = (inputField_ ? inputField_->geometry().bottom() : 0)
                       + (infoLabel_  ? infoLabel_->geometry().height() : 0) + 24;
    const int drawTop = qMax(controlH, 80);

    const QRect timelineRect(10, drawTop, w / 2 - 15, h - drawTop - 10);
    const QRect chartRect(w / 2 + 5, drawTop, w / 2 - 15, (h - drawTop - 10) / 2 - 5);
    const QRect statsRect(w / 2 + 5, drawTop + (h - drawTop - 10) / 2 + 5,
                          w / 2 - 15, (h - drawTop - 10) / 2 - 5);

    drawRetreatTimeline(p, timelineRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingRetreat::drawRetreatTimeline(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Retreat Timeline"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No retreat entries yet."));
        return;
    }

    QFont normal = p.font();
    normal.setBold(false);
    normal.setPointSize(9);
    p.setFont(normal);

    const int leftMargin = 14;
    const int topMargin = 40;
    const int barHeight = 18;
    const int gap = 6;
    const qreal maxHours = [&]() {
        qreal m = 1.0;
        for (const auto& e : entries_)
            m = qMax(m, static_cast<qreal>(e.hours));
        return m;
    }();

    const int usableW = rect.width() - leftMargin - 60;
    int y = rect.y() + topMargin;

    const int maxVisible = qMax(1, (rect.height() - topMargin - 10) / (barHeight + gap));
    const int startIdx = qMax(0, entries_.size() - maxVisible);

    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        const int bw = static_cast<int>(usableW * (e.hours / maxHours));

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + leftMargin, y, bw, barHeight, 3, 3);

        p.setPen(QColor("#1e293b"));
        p.drawText(QRect(rect.x() + leftMargin + bw + 6, y - 1, usableW - bw, barHeight + 2),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2h  depth:%3%4")
                       .arg(e.topic)
                       .arg(e.hours)
                       .arg(e.depth, 0, 'f', 1)
                       .arg(e.completed ? " [done]" : ""));

        y += barHeight + gap;
    }
}

void PaperReadingRetreat::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Category Breakdown"));

    const auto counts = categoryCounts();
    if (counts.isEmpty())
        return;

    const int cx = rect.x() + rect.width() / 2;
    const int cy = rect.y() + rect.height() / 2 + 8;
    const int outerR = qMin(rect.width(), rect.height()) / 2 - 24;
    const int innerR = outerR * 2 / 3;

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    qreal angle = 0.0;
    int colorIdx = 0;

    QFont labelFont = p.font();
    labelFont.setBold(false);
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const qreal span = 360.0 * it.value() / static_cast<qreal>(total);
        const QColor c = kPalette.at(colorIdx % kPalette.size());

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(angle * 16), static_cast<int>(span * 16));

        p.setBrush(QColor("#ffffff"));
        p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        const qreal midAngle = qDegreesToRadians(angle + span / 2.0);
        const int labelR = outerR + 14;
        const int lx = cx + static_cast<int>(labelR * qCos(midAngle));
        const int ly = cy - static_cast<int>(labelR * qSin(midAngle));

        p.setPen(QColor("#374151"));
        p.drawText(QRect(lx - 50, ly - 8, 100, 16), Qt::AlignCenter,
                   QStringLiteral("%1 (%2)").arg(it.key()).arg(it.value()));

        angle += span;
        ++colorIdx;
    }
}

void PaperReadingRetreat::drawStats(QPainter& p, const QRect& rect)
{
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Statistics"));

    QFont body = p.font();
    body.setBold(false);
    body.setPointSize(10);
    p.setFont(body);

    const int totalHours = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.hours;
        return s;
    }();

    const int left = rect.x() + 14;
    int y = rect.y() + 36;
    const int lineH = 22;

    auto drawLine = [&](const QString& label, const QString& value) {
        p.setPen(QColor("#64748b"));
        p.drawText(left, y, label);
        p.setPen(QColor("#0f172a"));
        p.drawText(left + 160, y, value);
        y += lineH;
    };

    drawLine(tr("Total entries:"), QString::number(entries_.size()));
    drawLine(tr("Completed:"), QString::number(completedCount()));
    drawLine(tr("Total hours:"), QString::number(totalHours));
    drawLine(tr("Avg depth:"), QString::number(avgDepth(), 'f', 1));
}

void PaperReadingRetreat::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No retreat entries yet. Click Start Retreat to begin."));
        return;
    }

    const int totalHours = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.hours;
        return s;
    }();

    infoLabel_->setText(tr("%1 entry(s) | %2 completed | %3 hours | avg depth %4")
        .arg(entries_.size())
        .arg(completedCount())
        .arg(totalHours)
        .arg(avgDepth(), 0, 'f', 1));
}

void PaperReadingRetreat::loadSettings()
{
    settings_.beginGroup("entries");
    const int size = settings_.beginReadArray("list");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        RetreatEntry e;
        e.id        = settings_.value("id").toInt();
        e.topic     = settings_.value("topic").toString();
        e.category  = settings_.value("category").toString();
        e.focus     = settings_.value("focus").toString();
        e.depth     = settings_.value("depth").toReal();
        e.hours     = settings_.value("hours").toInt();
        e.completed = settings_.value("completed").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    updateInfo();
    update();
}

void PaperReadingRetreat::saveSettings()
{
    settings_.beginGroup("entries");
    settings_.beginWriteArray("list", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",        e.id);
        settings_.setValue("topic",     e.topic);
        settings_.setValue("category",  e.category);
        settings_.setValue("focus",     e.focus);
        settings_.setValue("depth",     e.depth);
        settings_.setValue("hours",     e.hours);
        settings_.setValue("completed", e.completed);
        settings_.setValue("color",     e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
