#include "reading/PaperReadingStaminaTracker.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
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

PaperReadingStaminaTracker::PaperReadingStaminaTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingStaminaTracker")
    , trackBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingStaminaTracker::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // --- Input row ---
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Machine Learning"));
    categoryCombo_->addItem(tr("NLP"));
    categoryCombo_->addItem(tr("Computer Vision"));
    categoryCombo_->addItem(tr("Systems"));
    categoryCombo_->addItem(tr("Theory"));
    categoryCombo_->addItem(tr("Other"));
    inputRow->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Session name..."));
    inputField_->setMinimumWidth(220);
    inputRow->addWidget(inputField_);

    trackBtn_ = new QPushButton(tr("Track"), this);
    trackBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    inputRow->addWidget(trackBtn_);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}"
        "QPushButton:pressed{background:#991b1b;}");
    inputRow->addWidget(clearBtn_);

    inputRow->addStretch(1);

    root->addLayout(inputRow);

    // --- Info label ---
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet("font-size:13px; color:#6b7280;");
    root->addWidget(infoLabel_);

    // --- Custom painted area stretches to fill remaining space ---
    root->addStretch(1);

    setMinimumSize(640, 420);

    connect(trackBtn_, &QPushButton::clicked, this, &PaperReadingStaminaTracker::onTrack);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingStaminaTracker::onClear);
}

// ---------- public API ----------

void PaperReadingStaminaTracker::addEntry(const StaminaEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<StaminaEntry> PaperReadingStaminaTracker::entries() const
{
    return entries_;
}

int PaperReadingStaminaTracker::sustainedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.sustained)
            ++count;
    }
    return count;
}

qreal PaperReadingStaminaTracker::avgStamina() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.stamina;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingStaminaTracker::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------- slots ----------

void PaperReadingStaminaTracker::onTrack()
{
    const QString session = inputField_->text().trimmed();
    if (session.isEmpty())
        return;

    auto* rng = QRandomGenerator::global();

    StaminaEntry entry;
    entry.id       = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.session  = session;
    entry.category = categoryCombo_->currentText();
    entry.period   = QDateTime::currentDateTime().toString(Qt::ISODate);
    entry.stamina  = rng->bounded(1000) / 1000.0; // 0.0 - 1.0
    entry.pagesRead = rng->bounded(1, 101);        // 1 - 100
    entry.sustained = (entry.stamina > 0.6);
    entry.color    = kPalette.at(rng->bounded(static_cast<int>(kPalette.size())));

    entries_.append(entry);

    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    emit staminaTracked(entry.id, entry.stamina);
}

void PaperReadingStaminaTracker::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------- painting ----------

void PaperReadingStaminaTracker::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Reserve top area for controls (already handled by layout).
    const int controlH = (inputField_ ? inputField_->geometry().bottom() : 0)
                       + (infoLabel_  ? infoLabel_->geometry().height() : 0) + 24;
    const int drawTop = qMax(controlH, 80);

    const int colW = (w - 30) / 3;
    const int drawH = h - drawTop - 10;

    // 3-column layout: stamina chart | category chart | stats
    const QRect staminaRect(10, drawTop, colW, drawH);
    const QRect categoryRect(20 + colW, drawTop, colW, drawH);
    const QRect statsRect(30 + colW * 2, drawTop, colW, drawH);

    drawStaminaChart(p, staminaRect);
    drawCategoryChart(p, categoryRect);
    drawStats(p, statsRect);
}

void PaperReadingStaminaTracker::drawStaminaChart(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Reading Stamina"));

    if (entries_.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No sessions tracked yet."));
        return;
    }

    const int leftMargin = 10;
    const int topMargin  = 40;
    const int bottomMargin = 24;
    const int rightMargin = 10;

    const QRect chartArea(rect.x() + leftMargin, rect.y() + topMargin,
                          rect.width() - leftMargin - rightMargin,
                          rect.height() - topMargin - bottomMargin);

    // Draw sustained zone (green band for stamina > 0.6)
    const qreal sustainedY = chartArea.y() + chartArea.height() * (1.0 - 0.6);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(22, 163, 106, 30)); // #16a34a with alpha
    p.drawRect(chartArea.x(), chartArea.y(),
               chartArea.width(), static_cast<int>(sustainedY - chartArea.y()));

    // Draw sustained threshold line
    p.setPen(QPen(QColor("#16a34a"), 1, Qt::DashLine));
    p.drawLine(chartArea.x(), static_cast<int>(sustainedY),
               chartArea.right(), static_cast<int>(sustainedY));

    // Draw area chart: stamina over sessions
    const int n = entries_.size();
    if (n < 1)
        return;

    const qreal dx = static_cast<qreal>(chartArea.width()) / qMax(n - 1, 1);

    // Build polygon for filled area
    QPolygonF areaPoly;
    areaPoly << QPointF(chartArea.x(), chartArea.bottom());
    for (int i = 0; i < n; ++i) {
        const qreal x = chartArea.x() + i * dx;
        const qreal y = chartArea.bottom() - entries_.at(i).stamina * chartArea.height();
        areaPoly << QPointF(x, y);
    }
    areaPoly << QPointF(chartArea.x() + (n - 1) * dx, chartArea.bottom());

    // Fill area
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246, 50)); // #3b82f6 with alpha
    p.drawPolygon(areaPoly);

    // Draw line on top
    p.setPen(QPen(QColor("#3b82f6"), 2));
    p.setBrush(Qt::NoBrush);
    for (int i = 0; i < n; ++i) {
        const qreal x = chartArea.x() + i * dx;
        const qreal y = chartArea.bottom() - entries_.at(i).stamina * chartArea.height();
        if (i == 0)
            p.drawLine(static_cast<int>(x), static_cast<int>(y),
                       static_cast<int>(x), static_cast<int>(y));
        else {
            const qreal px = chartArea.x() + (i - 1) * dx;
            const qreal py = chartArea.bottom() - entries_.at(i - 1).stamina * chartArea.height();
            p.drawLine(static_cast<int>(px), static_cast<int>(py),
                       static_cast<int>(x), static_cast<int>(y));
        }
    }

    // Draw dots at each data point
    p.setBrush(QColor("#3b82f6"));
    for (int i = 0; i < n; ++i) {
        const qreal x = chartArea.x() + i * dx;
        const qreal y = chartArea.bottom() - entries_.at(i).stamina * chartArea.height();
        p.setPen(Qt::NoPen);
        p.drawEllipse(QPointF(x, y), 3, 3);
    }

    // Y-axis labels
    QFont smallFont = p.font();
    smallFont.setBold(false);
    smallFont.setPointSize(7);
    p.setFont(smallFont);
    p.setPen(QColor("#94a3b8"));

    p.drawText(chartArea.x() - 2, chartArea.y() - 2, tr("1.0"));
    p.drawText(chartArea.x() - 2, chartArea.bottom() + 10, tr("0.0"));
    p.drawText(chartArea.x() - 2, static_cast<int>(sustainedY) - 2, tr("0.6"));
}

void PaperReadingStaminaTracker::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(10, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop, tr("Categories"));

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont normal = p.font();
        normal.setBold(false);
        normal.setPointSize(10);
        p.setFont(normal);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(10, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No category data yet."));
        return;
    }

    // Find max count for scaling
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());
    maxCount = qMax(maxCount, 1);

    const int leftMargin = 10;
    const int topMargin  = 40;
    const int rightMargin = 10;
    const int barHeight  = 20;
    const int gap        = 6;
    const int labelWidth = 100;

    const int usableW = rect.width() - leftMargin - rightMargin - labelWidth - 10;
    int y = rect.y() + topMargin;
    int colorIdx = 0;

    QFont barFont = p.font();
    barFont.setBold(false);
    barFont.setPointSize(9);
    p.setFont(barFont);

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + barHeight > rect.bottom() - 5)
            break;

        // Category label
        p.setPen(QColor("#374151"));
        p.drawText(QRect(rect.x() + leftMargin, y - 1, labelWidth, barHeight + 2),
                   Qt::AlignVCenter | Qt::AlignRight, it.key());

        // Bar
        const int bw = static_cast<int>(usableW * (static_cast<qreal>(it.value()) / maxCount));
        const QColor c = kPalette.at(colorIdx % kPalette.size());

        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + leftMargin + labelWidth + 6, y, bw, barHeight, 3, 3);

        // Count label
        p.setPen(QColor("#1e293b"));
        p.drawText(QRect(rect.x() + leftMargin + labelWidth + 6 + bw + 6, y - 1,
                         50, barHeight + 2),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(it.value()));

        y += barHeight + gap;
        ++colorIdx;
    }
}

void PaperReadingStaminaTracker::drawStats(QPainter& p, const QRect& rect)
{
    // Card background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
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

    const int totalSessions = entries_.size();
    const int sustained = sustainedCount();
    const qreal avg = avgStamina();

    const int totalPages = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.pagesRead;
        return s;
    }();

    const int left = rect.x() + 14;
    int y = rect.y() + 36;
    const int lineH = 22;

    auto drawLine = [&](const QString& label, const QString& value) {
        p.setPen(QColor("#64748b"));
        p.drawText(left, y, label);
        p.setPen(QColor("#0f172a"));
        p.drawText(left + 130, y, value);
        y += lineH;
    };

    drawLine(tr("Total sessions:"), QString::number(totalSessions));
    drawLine(tr("Sustained:"),      QString::number(sustained));
    drawLine(tr("Avg stamina:"),    QString::number(avg, 'f', 2));
    drawLine(tr("Total pages:"),    QString::number(totalPages));

    if (totalSessions > 0) {
        const qreal sustainRate = static_cast<qreal>(sustained) / static_cast<qreal>(totalSessions) * 100.0;
        drawLine(tr("Sustain rate:"), QString::number(sustainRate, 'f', 1) + QStringLiteral("%"));
    }

    // Mini stamina indicator bar
    if (totalSessions > 0) {
        y += 8;
        const int barW = rect.width() - 28;
        const int barH = 10;

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(left, y, barW, barH, 5, 5);

        // Fill based on avg stamina
        const int fillW = static_cast<int>(barW * avg);
        const QColor fillColor = avg > 0.6 ? QColor("#16a34a") :
                                  avg > 0.3 ? QColor("#d97706") : QColor("#dc2626");
        p.setBrush(fillColor);
        p.drawRoundedRect(left, y, fillW, barH, 5, 5);
    }
}

// ---------- helpers ----------

void PaperReadingStaminaTracker::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No reading stamina sessions tracked."));
        return;
    }

    infoLabel_->setText(tr("Sessions: %1 | Sustained: %2 | Avg Stamina: %3")
        .arg(entries_.size())
        .arg(sustainedCount())
        .arg(avgStamina(), 0, 'f', 2));
}

void PaperReadingStaminaTracker::loadSettings()
{
    settings_.beginGroup("ReadingStaminaTracker");
    const int size = settings_.beginReadArray("entries");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StaminaEntry e;
        e.id        = settings_.value("id").toInt();
        e.session   = settings_.value("session").toString();
        e.category  = settings_.value("category").toString();
        e.period    = settings_.value("period").toString();
        e.stamina   = settings_.value("stamina").toReal();
        e.pagesRead = settings_.value("pagesRead").toInt();
        e.sustained = settings_.value("sustained").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();

    updateInfo();
    update();
}

void PaperReadingStaminaTracker::saveSettings()
{
    settings_.beginGroup("ReadingStaminaTracker");
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",        e.id);
        settings_.setValue("session",   e.session);
        settings_.setValue("category",  e.category);
        settings_.setValue("period",    e.period);
        settings_.setValue("stamina",   e.stamina);
        settings_.setValue("pagesRead", e.pagesRead);
        settings_.setValue("sustained", e.sustained);
        settings_.setValue("color",     e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
