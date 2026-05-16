#include "reading/PaperReadingEndurance2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QFont>
#include <QtMath>
#include <QPainterPath>

// ---------- Palette ----------
static const QList<QColor> kPalette = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};

static const QStringList kModes       = {"Sprint", "Marathon", "Interval", "Cruise"};
static const QStringList kCategories  = {"Literature", "Science", "Engineering", "Math", "History"};

static QColor colorForMode(const QString& mode)
{
    int idx = kModes.indexOf(mode);
    if (idx >= 0) return kPalette[idx % kPalette.size()];
    return QColor("#6b7280");
}

// ---------- Construction ----------

PaperReadingEndurance2::PaperReadingEndurance2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingEndurance2")
    , startBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperReadingEndurance2::setupUI()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(12, 12, 12, 12);
    root->setSpacing(8);

    // --- Input row ---
    auto* inputRow = new QHBoxLayout;
    inputRow->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    for (const auto& cat : kCategories)
        categoryCombo_->addItem(cat);
    inputRow->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Session name..."));
    inputField_->setMinimumWidth(220);
    inputRow->addWidget(inputField_);

    startBtn_ = new QPushButton(tr("Start"), this);
    startBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;border:none;border-radius:4px;"
        "padding:6px 16px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}"
        "QPushButton:pressed{background:#1d4ed8;}");
    inputRow->addWidget(startBtn_);

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

    // --- Painted area stretches ---
    root->addStretch(1);
    setMinimumSize(720, 520);

    connect(startBtn_, &QPushButton::clicked, this, &PaperReadingEndurance2::onStart);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingEndurance2::onClear);
}

// ---------- Public API ----------

void PaperReadingEndurance2::addEntry(const ReadingEndurance2Entry& entry)
{
    entries_.append(entry);
    updateInfo();
    saveSettings();
    update();
}

QList<ReadingEndurance2Entry> PaperReadingEndurance2::entries() const
{
    return entries_;
}

int PaperReadingEndurance2::marathonCount() const
{
    int c = 0;
    for (const auto& e : entries_)
        if (e.marathon) ++c;
    return c;
}

qreal PaperReadingEndurance2::avgStamina() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.stamina;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperReadingEndurance2::categoryCounts() const
{
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

// ---------- Slots ----------

void PaperReadingEndurance2::onStart()
{
    auto* rng = QRandomGenerator::global();

    ReadingEndurance2Entry e;
    e.id       = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    e.session  = inputField_->text().trimmed();
    if (e.session.isEmpty())
        e.session = QString("Session-%1").arg(e.id);
    e.category = categoryCombo_->currentText();
    e.mode     = kModes.at(rng->bounded(kModes.size()));
    e.stamina  = rng->bounded(1000) / 1000.0;
    e.pages    = rng->bounded(5, 151);
    e.marathon = (e.mode == "Marathon" && e.pages >= 40);
    e.color    = colorForMode(e.mode);

    entries_.append(e);
    inputField_->clear();
    updateInfo();
    saveSettings();
    update();

    emit sessionComplete(e.id, e.stamina);
}

void PaperReadingEndurance2::onClear()
{
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

// ---------- Painting ----------

void PaperReadingEndurance2::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    const int w = width();
    const int h = height();

    // Background
    p.fillRect(rect(), QColor("#f8fafc"));

    // Reserve space for controls above the painted area
    const int controlH = (inputField_ ? inputField_->geometry().bottom() : 0)
                       + (infoLabel_  ? infoLabel_->geometry().height() : 0) + 24;
    const int drawTop = qMax(controlH, 80);
    const int drawH   = h - drawTop - 10;

    // Top half: session cards
    const QRect enduranceRect(10, drawTop, w - 20, drawH / 2 - 5);

    // Bottom half: category donut (left) + stats (right)
    const int bottomY = drawTop + drawH / 2 + 5;
    const int halfW   = (w - 30) / 2;
    const QRect categoryRect(10, bottomY, halfW, drawH / 2 - 5);
    const QRect statsRect(20 + halfW, bottomY, halfW, drawH / 2 - 5);

    drawEnduranceView(p, enduranceRect);
    drawCategoryChart(p, categoryRect);
    drawStats(p, statsRect);
}

// --- Session cards with mode badge, heart-rate stamina bar, page count, trophy ---

void PaperReadingEndurance2::drawEnduranceView(QPainter& p, const QRect& rect)
{
    // Card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Section title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Reading Endurance"));

    if (entries_.isEmpty()) {
        QFont hint = p.font();
        hint.setBold(false);
        hint.setPointSize(10);
        p.setFont(hint);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(12, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No endurance sessions yet. Click Start to begin."));
        return;
    }

    // Layout: horizontal scrolling session cards
    const int leftM   = 12;
    const int topM    = 36;
    const int rightM  = 12;
    const int bottomM = 8;
    const int cardGap = 10;
    const int cardW   = 130;

    const int usableW = rect.width() - leftM - rightM;
    const int usableH = rect.height() - topM - bottomM;
    const int cardH   = usableH - 4;

    const int maxVisible = qMax(1, (usableW + cardGap) / (cardW + cardGap));
    const int startIdx   = qMax(0, entries_.size() - maxVisible);
    const int visCount   = entries_.size() - startIdx;
    if (visCount <= 0) return;

    QFont smallFont;
    smallFont.setPointSize(7);
    p.setFont(smallFont);

    for (int vi = 0; vi < visCount; ++vi) {
        const int idx = startIdx + vi;
        const auto& e = entries_.at(idx);

        const int cx = rect.x() + leftM + vi * (cardW + cardGap);
        const int cy = rect.y() + topM;

        // Card background with subtle border
        p.setPen(QPen(QColor("#e2e8f0"), 1));
        p.setBrush(QColor("#f9fafb"));
        p.drawRoundedRect(cx, cy, cardW, cardH, 6, 6);

        // ---- Mode badge (top-left) ----
        const QColor modeCol = colorForMode(e.mode);
        const QString badgeText = e.mode;
        QFontMetrics bfm(p.font());
        const int badgeW = bfm.horizontalAdvance(badgeText) + 12;
        const int badgeH = 16;
        p.setPen(Qt::NoPen);
        p.setBrush(modeCol);
        p.drawRoundedRect(cx + 6, cy + 6, badgeW, badgeH, badgeH / 2, badgeH / 2);
        p.setPen(QColor("#ffffff"));
        p.setFont(smallFont);
        p.drawText(QRect(cx + 6, cy + 6, badgeW, badgeH),
                   Qt::AlignCenter, badgeText);

        // ---- Marathon trophy icon (top-right) ----
        if (e.marathon) {
            const int tx = cx + cardW - 22;
            const int ty = cy + 6;
            // Trophy body (cup shape)
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#d97706"));
            // Cup
            p.drawRoundedRect(tx, ty, 14, 12, 2, 2);
            // Cup rim
            p.setBrush(QColor("#f59e0b"));
            p.drawRect(tx + 2, ty, 10, 3);
            // Stem
            p.setBrush(QColor("#d97706"));
            p.drawRect(tx + 5, ty + 12, 4, 4);
            // Base
            p.drawRect(tx + 3, ty + 16, 8, 2);
            // Handles
            p.setPen(QPen(QColor("#f59e0b"), 1.5));
            p.setBrush(Qt::NoBrush);
            p.drawArc(tx - 3, ty + 1, 7, 8, -30 * 16, -120 * 16);
            p.drawArc(tx + 11, ty + 1, 7, 8, 210 * 16, -120 * 16);
        }

        // ---- Session name ----
        p.setPen(QColor("#1e293b"));
        QFont nameFont;
        nameFont.setPointSize(8);
        nameFont.setBold(true);
        p.setFont(nameFont);
        QFontMetrics nfm(nameFont);
        const QString shortName = nfm.elidedText(e.session, Qt::ElideRight, cardW - 16);
        p.drawText(QRect(cx + 6, cy + 26, cardW - 16, 16),
                   Qt::AlignLeft | Qt::AlignVCenter, shortName);

        // ---- Stamina bar (heart-rate style: ECG waveform + fill) ----
        const int barX = cx + 8;
        const int barY = cy + 48;
        const int barW = cardW - 16;
        const int barH = 14;

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#e2e8f0"));
        p.drawRoundedRect(barX, barY, barW, barH, barH / 2, barH / 2);

        // Filled portion
        const int fillW = static_cast<int>(barW * qBound(0.0, e.stamina, 1.0));
        if (fillW > 0) {
            QColor fillCol = modeCol;
            fillCol.setAlpha(210);
            p.setBrush(fillCol);
            p.drawRoundedRect(barX, barY, qMax(fillW, barH), barH, barH / 2, barH / 2);

            // Heart rate ECG zigzag on the fill
            QPen ecgPen(QColor(255, 255, 255, 180), 1.2);
            p.setPen(ecgPen);
            p.setBrush(Qt::NoBrush);
            const int midY = barY + barH / 2;
            QPainterPath ecg;
            ecg.moveTo(barX + 2, midY);
            const int waveCount = qMax(2, fillW / 10);
            const int waveW = fillW / waveCount;
            for (int wi = 0; wi < waveCount; ++wi) {
                const int wx = barX + 2 + wi * waveW;
                // Flat -> up peak -> down valley -> flat (ECG PQRST shape)
                ecg.lineTo(wx + waveW * 0.2, midY);
                ecg.lineTo(wx + waveW * 0.35, midY - barH * 0.35);
                ecg.lineTo(wx + waveW * 0.45, midY + barH * 0.4);
                ecg.lineTo(wx + waveW * 0.55, midY - barH * 0.15);
                ecg.lineTo(wx + waveW * 0.7, midY);
                ecg.lineTo(wx + waveW, midY);
            }
            p.drawPath(ecg);
        }

        // Stamina percentage
        p.setPen(QColor("#64748b"));
        QFont pctFont;
        pctFont.setPointSize(7);
        p.setFont(pctFont);
        p.drawText(QRect(barX, barY + barH + 2, barW, 12),
                   Qt::AlignCenter,
                   QString::number(static_cast<int>(e.stamina * 100)) + QStringLiteral("% stamina"));

        // ---- Heart icon before stamina text ----
        {
            const int heartCx = barX + barW / 2 - 32;
            const int heartCy = barY + barH + 3;
            p.setPen(Qt::NoPen);
            p.setBrush(e.stamina > 0.6 ? QColor("#dc2626") :
                       e.stamina > 0.3 ? QColor("#d97706") : QColor("#94a3b8"));
            // Simple heart shape using two arcs + triangle
            QPainterPath heart;
            heart.moveTo(heartCx, heartCy + 3);
            heart.cubicTo(heartCx, heartCy, heartCx - 5, heartCy - 2, heartCx, heartCy + 1);
            heart.cubicTo(heartCx + 5, heartCy - 2, heartCx, heartCy, heartCx, heartCy + 3);
            heart.closeSubpath();
            p.drawPath(heart);
        }

        // ---- Page count ----
        p.setPen(QColor("#374151"));
        QFont pageFont;
        pageFont.setPointSize(8);
        pageFont.setBold(true);
        p.setFont(pageFont);
        p.drawText(QRect(cx + 6, cy + cardH - 28, cardW - 16, 18),
                   Qt::AlignCenter,
                   QString::number(e.pages) + QStringLiteral(" pages"));

        // Category label
        p.setPen(QColor("#94a3b8"));
        QFont catFont;
        catFont.setPointSize(6);
        p.setFont(catFont);
        p.drawText(QRect(cx + 6, cy + cardH - 14, cardW - 16, 12),
                   Qt::AlignCenter, e.category);
    }
}

// --- Donut chart for category breakdown ---

void PaperReadingEndurance2::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Category Breakdown"));

    const auto counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont hint = p.font();
        hint.setBold(false);
        hint.setPointSize(10);
        p.setFont(hint);
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.adjusted(12, 36, -10, 0), Qt::AlignLeft | Qt::AlignTop,
                   tr("No category data yet."));
        return;
    }

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();
    if (total == 0) return;

    const int cx = rect.x() + rect.width() / 2;
    const int cy = rect.y() + rect.height() / 2 + 8;
    const int outerR = qMin(rect.width(), rect.height()) / 2 - 30;
    const int innerR = static_cast<int>(outerR * 0.55);

    // Draw donut slices using proper arc segments
    qreal angle = 90.0;  // Start from top (12 o'clock)

    QFont labelFont;
    labelFont.setPointSize(8);
    labelFont.setBold(false);
    p.setFont(labelFont);

    int colorIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        const qreal span = 360.0 * static_cast<qreal>(it.value()) / static_cast<qreal>(total);
        const QColor c = kPalette[colorIdx % kPalette.size()];

        // Outer arc segment
        p.setPen(Qt::NoPen);
        p.setBrush(c);

        QPainterPath slice;
        const QRectF outerRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2);
        slice.moveTo(cx + innerR * qCos(qDegreesToRadians(angle)),
                     cy - innerR * qSin(qDegreesToRadians(angle)));
        slice.arcTo(outerRect, angle, -span);
        slice.lineTo(cx + innerR * qCos(qDegreesToRadians(angle - span)),
                     cy - innerR * qSin(qDegreesToRadians(angle - span)));
        const QRectF innerRect(cx - innerR, cy - innerR, innerR * 2, innerR * 2);
        slice.arcTo(innerRect, angle - span, span);
        slice.closeSubpath();
        p.drawPath(slice);

        // Label on slice midpoint
        const qreal midAngle = qDegreesToRadians(angle - span / 2.0);
        const qreal labelR = (outerR + innerR) / 2.0;
        const qreal lx = cx + labelR * qCos(midAngle);
        const qreal ly = cy - labelR * qSin(midAngle);

        p.setPen(QColor("#ffffff"));
        QFontMetrics lfm(labelFont);
        const QString lbl = it.key();
        const QString val = QString::number(it.value());
        p.drawText(QRect(static_cast<int>(lx) - 30, static_cast<int>(ly) - 8, 60, 16),
                   Qt::AlignCenter, lbl + QStringLiteral(" ") + val);

        angle -= span;
        ++colorIdx;
    }

    // Inner circle (donut hole)
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text: total count
    p.setPen(QColor("#1e293b"));
    QFont centerFont;
    centerFont.setBold(true);
    centerFont.setPointSize(14);
    p.setFont(centerFont);
    p.drawText(QRect(cx - innerR, cy - 14, innerR * 2, 24),
               Qt::AlignCenter, QString::number(total));
    centerFont.setBold(false);
    centerFont.setPointSize(8);
    p.setFont(centerFont);
    p.setPen(QColor("#64748b"));
    p.drawText(QRect(cx - innerR, cy + 8, innerR * 2, 16),
               Qt::AlignCenter, tr("sessions"));
}

// --- 4 stat boxes ---

void PaperReadingEndurance2::drawStats(QPainter& p, const QRect& rect)
{
    // Card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#ffffff"));
    p.drawRoundedRect(rect, 8, 8);

    // Title
    p.setPen(QColor("#1e293b"));
    QFont heading = p.font();
    heading.setBold(true);
    heading.setPointSize(11);
    p.setFont(heading);
    p.drawText(rect.adjusted(12, 8, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Statistics"));

    // Compute values
    const int totalSessions = entries_.size();
    const int marathons     = marathonCount();
    const qreal avg         = avgStamina();
    const int totalPages    = [&]() {
        int s = 0;
        for (const auto& e : entries_) s += e.pages;
        return s;
    }();

    // 4 stat boxes in a 2x2 grid
    struct StatBox { QString label; QString value; QColor accent; };
    const StatBox boxes[4] = {
        {tr("Total Sessions"), QString::number(totalSessions), QColor("#3b82f6")},
        {tr("Marathons"),      QString::number(marathons),     QColor("#7c3aed")},
        {tr("Avg Stamina"),    QString::number(avg, 'f', 2),   QColor("#16a34a")},
        {tr("Total Pages"),    QString::number(totalPages),    QColor("#d97706")}
    };

    const int boxMargin = 10;
    const int titleH    = 32;
    const int gridTop   = rect.y() + titleH + 8;
    const int gridW     = rect.width() - 2 * boxMargin;
    const int gridH     = rect.height() - titleH - 16;
    const int boxW      = (gridW - boxMargin) / 2;
    const int boxH      = (gridH - boxMargin) / 2;

    QFont boxFont;
    for (int i = 0; i < 4; ++i) {
        const int col = i % 2;
        const int row = i / 2;
        const int bx = rect.x() + boxMargin + col * (boxW + boxMargin);
        const int by = gridTop + row * (boxH + boxMargin);

        // Box background
        p.setPen(Qt::NoPen);
        boxes[i].accent.setAlpha(25);
        p.setBrush(boxes[i].accent);
        p.drawRoundedRect(bx, by, boxW, boxH, 6, 6);

        // Top accent stripe
        boxes[i].accent.setAlpha(255);
        p.setBrush(boxes[i].accent);
        p.drawRoundedRect(bx, by, boxW, 4, 2, 2);

        // Value
        p.setPen(QColor("#0f172a"));
        QFont valFont;
        valFont.setBold(true);
        valFont.setPointSize(qMax(10, qMin(boxH / 3, 18)));
        p.setFont(valFont);
        p.drawText(QRect(bx, by + 8, boxW, boxH / 2),
                   Qt::AlignCenter, boxes[i].value);

        // Label
        p.setPen(QColor("#64748b"));
        QFont lblFont;
        lblFont.setPointSize(8);
        lblFont.setBold(false);
        p.setFont(lblFont);
        p.drawText(QRect(bx, by + boxH / 2 + 4, boxW, boxH / 2 - 12),
                   Qt::AlignCenter, boxes[i].label);
    }

    // Marathon rate footer if sessions exist
    if (totalSessions > 0) {
        const qreal rate = static_cast<qreal>(marathons) / static_cast<qreal>(totalSessions) * 100.0;
        p.setPen(QColor("#94a3b8"));
        QFont footFont;
        footFont.setPointSize(7);
        p.setFont(footFont);
        p.drawText(rect.adjusted(12, -16, -12, 0),
                   Qt::AlignBottom | Qt::AlignRight,
                   tr("Marathon rate: %1%").arg(QString::number(rate, 'f', 1)));
    }
}

// ---------- Helpers ----------

void PaperReadingEndurance2::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No endurance sessions tracked."));
        return;
    }
    infoLabel_->setText(tr("Sessions: %1 | Marathons: %2 | Avg Stamina: %3")
        .arg(entries_.size())
        .arg(marathonCount())
        .arg(avgStamina(), 0, 'f', 2));
}

void PaperReadingEndurance2::loadSettings()
{
    settings_.beginGroup("ReadingEndurance2");
    const int size = settings_.beginReadArray("entries");
    entries_.clear();
    entries_.reserve(size);
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ReadingEndurance2Entry e;
        e.id       = settings_.value("id").toInt();
        e.session  = settings_.value("session").toString();
        e.category = settings_.value("category").toString();
        e.mode     = settings_.value("mode").toString();
        e.stamina  = settings_.value("stamina").toReal();
        e.pages    = settings_.value("pages").toInt();
        e.marathon = settings_.value("marathon").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperReadingEndurance2::saveSettings()
{
    settings_.beginGroup("ReadingEndurance2");
    settings_.beginWriteArray("entries", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_.at(i);
        settings_.setArrayIndex(i);
        settings_.setValue("id",       e.id);
        settings_.setValue("session",  e.session);
        settings_.setValue("category", e.category);
        settings_.setValue("mode",     e.mode);
        settings_.setValue("stamina",  e.stamina);
        settings_.setValue("pages",    e.pages);
        settings_.setValue("marathon", e.marathon);
        settings_.setValue("color",    e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
