#include "reading/PaperReadingBinge.hpp"
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

PaperReadingBinge::PaperReadingBinge(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingBinge")
    , recordBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingBinge::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // --- Input row ---
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(6);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter session name..."));
    inputField_->setMinimumWidth(220);
    inputRow->addWidget(inputField_);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Machine Learning"));
    categoryCombo_->addItem(tr("NLP"));
    categoryCombo_->addItem(tr("Computer Vision"));
    categoryCombo_->addItem(tr("Systems"));
    categoryCombo_->addItem(tr("Theory"));
    categoryCombo_->addItem(tr("Other"));
    inputRow->addWidget(categoryCombo_);

    recordBtn_ = new QPushButton(tr("Record"), this);
    recordBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    inputRow->addWidget(recordBtn_);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:pressed{background:#991b1b;}");
    inputRow->addWidget(clearBtn_);

    root->addLayout(inputRow);

    // --- Info label ---
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("font-size:13px; color:#6b7280;");
    root->addWidget(infoLabel_);

    // --- Custom painted area stretches to fill remaining space ---
    root->addStretch(1);

    setMinimumSize(640, 420);

    connect(recordBtn_, &QPushButton::clicked, this, &PaperReadingBinge::onRecord);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingBinge::onClear);
}

// ---------- public API ----------

void PaperReadingBinge::addEntry(const BingeEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<BingeEntry> PaperReadingBinge::entries() const
{
    return entries_;
}

int PaperReadingBinge::marathonCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.marathon)
            ++count;
    }
    return count;
}

qreal PaperReadingBinge::avgScore() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.score;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingBinge::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------- slots ----------

void PaperReadingBinge::onRecord()
{
    const QString session = inputField_->text().trimmed();
    if (session.isEmpty())
        return;

    auto* rng = QRandomGenerator::global();

    BingeEntry entry;
    entry.id       = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.session  = session;
    entry.category = categoryCombo_->currentText();

    // Deterministic-ish mode selection based on random hours
    const QStringList modes = {"deep-read", "skimming", "note-taking", "review"};
    entry.mode = modes.at(rng->bounded(static_cast<int>(modes.size())));

    entry.papers = rng->bounded(1, 21);          // 1-20 papers
    entry.hours  = rng->bounded(10, 800) / 100.0; // 0.1 - 7.9 hours
    entry.pages  = entry.papers * rng->bounded(5, 25); // pages scale with papers
    entry.score  = rng->bounded(50, 100) / 10.0;  // 5.0 - 9.9

    entry.marathon = (entry.hours > 4.0);

    entry.color = kPalette.at(rng->bounded(static_cast<int>(kPalette.size())));

    entries_.append(entry);

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    emit bingeRecorded(entry.id, entry.score);
}

void PaperReadingBinge::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------- painting ----------

void PaperReadingBinge::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Reserve top area for controls (already handled by layout).
    // Paint custom visuals in the remaining space below the info label.
    const int controlH = (inputField_ ? inputField_->geometry().bottom() : 0)
                       + (infoLabel_  ? infoLabel_->geometry().height() : 0) + 24;
    const int drawTop = qMax(controlH, 80);

    const QRect bingeRect(10, drawTop, w / 2 - 15, h - drawTop - 10);
    const QRect chartRect(w / 2 + 5, drawTop, w / 2 - 15, (h - drawTop - 10) / 2 - 5);
    const QRect statsRect(w / 2 + 5, drawTop + (h - drawTop - 10) / 2 + 5,
                          w / 2 - 15, (h - drawTop - 10) / 2 - 5);

    drawBingeView(p, bingeRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperReadingBinge::drawBingeView(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Reading Binge Log"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No sessions recorded yet."));
        return;
    }

    QFont normal = p.font();
    normal.setBold(false);
    normal.setPointSize(9);
    p.setFont(normal);

    // Draw horizontal bar chart of hours per session
    const int leftMargin = 10;
    const int topMargin  = 36;
    const int barHeight  = 16;
    const int gap        = 4;
    const qreal maxHours = [&]() {
        qreal m = 1.0;
        for (const auto& e : entries_)
            m = qMax(m, e.hours);
        return m;
    }();

    const int usableW = rect.width() - leftMargin - 10;
    int y = rect.y() + topMargin;

    // Show the most recent entries that fit
    const int maxVisible = qMax(1, (rect.height() - topMargin - 10) / (barHeight + gap));
    const int startIdx   = qMax(0, entries_.size() - maxVisible);

    for (int i = startIdx; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        const int bw   = static_cast<int>(usableW * (e.hours / maxHours));

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + leftMargin, y, bw, barHeight, 3, 3);

        // Label
        p.setPen(QColor("#1e293b"));
        p.drawText(QRect(rect.x() + leftMargin + bw + 6, y - 1, usableW - bw, barHeight + 2),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QStringLiteral("%1  %2h  %3papers%4")
                       .arg(e.session)
                       .arg(e.hours, 0, 'f', 1)
                       .arg(e.papers)
                       .arg(e.marathon ? " [marathon]" : ""));

        y += barHeight + gap;
    }
}

void PaperReadingBinge::drawCategoryChart(QPainter& p, const QRect& rect)
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

    // Simple donut chart
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

        // Inner circle to create donut
        p.setBrush(QColor("#ffffff"));
        p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

        // Label outside
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

void PaperReadingBinge::drawStats(QPainter& p, const QRect& rect)
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

    const int totalPapers = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.papers;
        return s;
    }();

    const qreal totalHours = [&]() {
        qreal s = 0;
        for (const auto& e : entries_) s += e.hours;
        return s;
    }();

    const qreal totalPages = [&]() {
        qreal s = 0;
        for (const auto& e : entries_) s += e.pages;
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

    drawLine(tr("Total sessions:"), QString::number(entries_.size()));
    drawLine(tr("Total papers:"),   QString::number(totalPapers));
    drawLine(tr("Total hours:"),    QString::number(totalHours, 'f', 1));
    drawLine(tr("Total pages:"),    QString::number(totalPages, 'f', 0));
    drawLine(tr("Marathon sessions:"), QString::number(marathonCount()));
    drawLine(tr("Avg score:"),      QString::number(avgScore(), 'f', 1));
}

// ---------- helpers ----------

void PaperReadingBinge::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No reading binge sessions recorded."));
        return;
    }

    const int totalPapers = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.papers;
        return s;
    }();

    infoLabel_->setText(tr("%1 session(s) | %2 paper(s) | %3 marathon(s) | avg score %4")
        .arg(entries_.size())
        .arg(totalPapers)
        .arg(marathonCount())
        .arg(avgScore(), 0, 'f', 1));
}

void PaperReadingBinge::loadSettings()
{
    settings_.beginGroup("entries");
    const int size = settings_.beginReadArray("list");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BingeEntry e;
        e.id       = settings_.value("id").toInt();
        e.session  = settings_.value("session").toString();
        e.category = settings_.value("category").toString();
        e.mode     = settings_.value("mode").toString();
        e.papers   = settings_.value("papers").toInt();
        e.hours    = settings_.value("hours").toReal();
        e.pages    = settings_.value("pages").toReal();
        e.score    = settings_.value("score").toReal();
        e.marathon = settings_.value("marathon").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    updateInfo();
    update();
}

void PaperReadingBinge::saveSettings()
{
    settings_.beginGroup("entries");
    settings_.beginWriteArray("list", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",       e.id);
        settings_.setValue("session",  e.session);
        settings_.setValue("category", e.category);
        settings_.setValue("mode",     e.mode);
        settings_.setValue("papers",   e.papers);
        settings_.setValue("hours",    e.hours);
        settings_.setValue("pages",    e.pages);
        settings_.setValue("score",    e.score);
        settings_.setValue("marathon", e.marathon);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
