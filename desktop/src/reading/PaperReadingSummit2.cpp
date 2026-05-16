#include "reading/PaperReadingSummit2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

namespace {
const QColor CLR_BLUE(0x3b82f6);
const QColor CLR_GREEN(0x16a34a);
const QColor CLR_AMBER(0xd97706);
const QColor CLR_RED(0xdc2626);
const QColor CLR_PURPLE(0x7c3aed);
const QColor CLR_BG(0xf8fafc);
const QColor CLR_TEXT(0x334155);
const QColor CLR_MUTED(0x94a3b8);
const QColor CLR_BORDER(0xe2e8f0);
const QColor CLR_CARD(0xffffff);
}

PaperReadingSummit2::PaperReadingSummit2(QWidget* parent)
    : QWidget(parent), settings_("PaperCrawler", "ReadingSummit2") {
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        static const QStringList peaks = {
            "Literature Review Peak", "Methodology Summit",
            "Results Ridge", "Discussion Dome"
        };
        static const QStringList categories = {
            "Survey", "Experiment", "Analysis", "Theory", "Review"
        };
        static const QStringList approaches = {
            "Direct", "Gradual", "Switchback", "Traverse"
        };
        QList<QColor> palette = {CLR_BLUE, CLR_GREEN, CLR_AMBER, CLR_RED, CLR_PURPLE};
        for (int i = 0; i < 8; ++i) {
            ReadingSummit2Entry e;
            e.id = i + 1;
            e.peak = peaks[i % peaks.size()];
            e.category = categories[i % categories.size()];
            e.approach = approaches[i % approaches.size()];
            e.altitude = 1000.0 + QRandomGenerator::global()->bounded(8000.0);
            e.stages = 1 + QRandomGenerator::global()->bounded(10);
            e.summited = QRandomGenerator::global()->bounded(3) == 0;
            e.color = palette[i % palette.size()];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperReadingSummit2::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    auto* toolbar = new QHBoxLayout;
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "Survey", "Experiment", "Analysis", "Theory", "Review"});
    categoryCombo_->setStyleSheet(
        "QComboBox{padding:5px 10px;border:1px solid #cbd5e1;border-radius:4px;"
        "background:#fff;min-width:90px;}"
        "QComboBox::drop-down{border:none;}");

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Search peaks...");
    inputField_->setStyleSheet(
        "QLineEdit{padding:5px 10px;border:1px solid #cbd5e1;border-radius:4px;"
        "background:#fff;}");

    climbBtn_ = new QPushButton("Climb", this);
    climbBtn_->setStyleSheet(
        "QPushButton{background:#3b82f6;color:#fff;padding:6px 14px;"
        "border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#2563eb;}");

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet(
        "QPushButton{background:#dc2626;color:#fff;padding:6px 14px;"
        "border-radius:4px;font-weight:bold;}"
        "QPushButton:hover{background:#b91c1c;}");

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("color:#64748b;font-size:11px;");

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_);
    toolbar->addWidget(climbBtn_);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    toolbar->addWidget(infoLabel_);

    mainLayout->addLayout(toolbar);

    connect(climbBtn_, &QPushButton::clicked, this, &PaperReadingSummit2::onClimb);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingSummit2::onClear);
}

void PaperReadingSummit2::addEntry(const ReadingSummit2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ReadingSummit2Entry> PaperReadingSummit2::entries() const { return entries_; }

int PaperReadingSummit2::summitedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.summited) ++c;
    return c;
}

qreal PaperReadingSummit2::avgAltitude() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.altitude;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingSummit2::categoryCounts() const {
    QMap<QString, int> m;
    for (const auto& e : entries_) m[e.category]++;
    return m;
}

void PaperReadingSummit2::onClimb() {
    static const QStringList peaks = {
        "Literature Review Peak", "Methodology Summit",
        "Results Ridge", "Discussion Dome"
    };
    static const QStringList categories = {
        "Survey", "Experiment", "Analysis", "Theory", "Review"
    };
    static const QStringList approaches = {
        "Direct", "Gradual", "Switchback", "Traverse"
    };
    QList<QColor> palette = {CLR_BLUE, CLR_GREEN, CLR_AMBER, CLR_RED, CLR_PURPLE};

    ReadingSummit2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.peak = inputField_->text().trimmed().isEmpty()
        ? peaks[QRandomGenerator::global()->bounded(peaks.size())]
        : inputField_->text().trimmed();
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.approach = approaches[QRandomGenerator::global()->bounded(approaches.size())];
    e.altitude = 1000.0 + QRandomGenerator::global()->bounded(8000.0);
    e.stages = 1 + QRandomGenerator::global()->bounded(10);
    e.summited = QRandomGenerator::global()->bounded(3) == 0;
    e.color = palette[e.id % palette.size()];
    entries_.append(e);
    emit peakReached(e.id, e.altitude);
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingSummit2::onClear() {
    entries_.clear();
    updateInfo();
    saveSettings();
    update();
}

void PaperReadingSummit2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    int w = width(), h = height();
    p.fillRect(rect(), CLR_BG);

    int toolbarH = 44;
    int contentH = h - toolbarH;
    int leftW = static_cast<int>(w * 0.6);
    int rightW = w - leftW;
    int bottomH = static_cast<int>(contentH * 0.25);
    int topH = contentH - bottomH;

    drawSummitView(p, QRect(0, toolbarH, leftW, topH));
    drawCategoryChart(p, QRect(leftW, toolbarH, rightW, topH));
    drawStats(p, QRect(0, toolbarH + topH, w, bottomH));
}

void PaperReadingSummit2::drawSummitView(QPainter& p, const QRect& rect) {
    // Section background card
    p.setPen(Qt::NoPen);
    p.setBrush(CLR_CARD);
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);

    p.setPen(CLR_TEXT);
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(14, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Summit View");

    if (entries_.isEmpty()) {
        p.setPen(CLR_MUTED);
        p.setFont(QFont("Sans", 9));
        p.drawText(rect, Qt::AlignCenter, "No peaks yet. Start climbing!");
        return;
    }

    // Filter by category
    QString filter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();

    QList<const ReadingSummit2Entry*> visible;
    for (const auto& e : entries_) {
        if (filter != "All" && e.category != filter) continue;
        if (!search.isEmpty() && !e.peak.toLower().contains(search)) continue;
        visible.append(&e);
    }

    int cardTop = rect.top() + 32;
    int cardAreaH = rect.height() - 40;
    int cardH = qMin(72, cardAreaH / qMax(1, visible.size()) - 4);
    if (cardH < 36) cardH = 36;

    qreal maxAlt = 9000.0;

    for (int i = 0; i < qMin(visible.size(), 12); ++i) {
        const auto& e = *visible[i];
        int y = cardTop + i * (cardH + 4);
        if (y + cardH > rect.bottom() - 4) break;

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xf1f5f9));
        p.drawRoundedRect(rect.left() + 12, y, rect.width() - 24, cardH, 6, 6);

        // Mountain silhouette fill (altitude progress)
        int barAreaW = rect.width() - 200;
        int barX = rect.left() + 120;
        int barY = y + 8;
        int barH = cardH - 16;

        // Background track
        p.setBrush(CLR_BORDER);
        p.drawRoundedRect(barX, barY, barAreaW, barH, 4, 4);

        // Mountain-shaped fill
        qreal ratio = qMin(e.altitude / maxAlt, 1.0);
        int fillW = static_cast<int>(ratio * barAreaW);
        if (fillW > 4) {
            QPainterPath mountainPath;
            int peakX = barX + fillW * 2 / 3;
            int baseY = barY + barH;
            int peakY = barY + 2;
            mountainPath.moveTo(barX, baseY);
            mountainPath.lineTo(barX + fillW * 1 / 3, baseY);
            mountainPath.lineTo(peakX, peakY);
            mountainPath.lineTo(barX + fillW, baseY);
            mountainPath.lineTo(barX + fillW, baseY);
            mountainPath.lineTo(barX, baseY);
            mountainPath.closeSubpath();

            p.setBrush(e.color);
            p.setOpacity(e.summited ? 0.9 : 0.5);
            p.drawPath(mountainPath);
            p.setOpacity(1.0);
        }

        // Altitude text on bar
        if (fillW > 50) {
            p.setPen(Qt::white);
            p.setFont(QFont("Sans", 8, QFont::Bold));
            p.drawText(QRect(barX, barY, fillW, barH),
                       Qt::AlignCenter, QString::number(e.altitude, 'f', 0));
        }

        // Peak name
        p.setPen(CLR_TEXT);
        p.setFont(QFont("Sans", 9, QFont::Bold));
        QString peakText = e.peak;
        if (peakText.length() > 16) peakText = peakText.left(14) + "...";
        p.drawText(QRect(rect.left() + 16, y + 4, 100, cardH / 2 - 2),
                   Qt::AlignLeft | Qt::AlignVCenter, peakText);

        // Approach badge
        p.setFont(QFont("Sans", 7));
        QFontMetrics fm(p.font());
        int badgeW = fm.horizontalAdvance(e.approach) + 10;
        int badgeX = rect.left() + 16;
        int badgeY = y + cardH / 2 + 2;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(140));
        p.drawRoundedRect(badgeX, badgeY, badgeW, 16, 8, 8);
        p.setPen(e.color);
        p.setFont(QFont("Sans", 7));
        p.drawText(QRect(badgeX, badgeY, badgeW, 16),
                   Qt::AlignCenter, e.approach);

        // Stage markers
        int markerStartX = barX + barAreaW + 8;
        p.setPen(CLR_MUTED);
        p.setFont(QFont("Sans", 7));
        int stagesShown = qMin(e.stages, 5);
        for (int s = 0; s < stagesShown; ++s) {
            int mx = markerStartX + s * 10;
            if (mx + 8 > rect.right() - 12) break;
            p.setBrush(s < e.stages ? e.color : CLR_BORDER);
            p.setPen(Qt::NoPen);
            p.drawEllipse(mx, y + cardH / 2 - 4, 8, 8);
        }
        p.setPen(CLR_MUTED);
        p.drawText(markerStartX, y + cardH / 2 + 14,
                   QString("x%1").arg(e.stages));

        // Summited flag
        if (e.summited) {
            p.setPen(QColor(0xdc2626));
            p.setFont(QFont("Sans", 11, QFont::Bold));
            p.drawText(rect.right() - 30, y + cardH / 2 + 4,
                       QChar(0x2691)); // flag
        }
    }
}

void PaperReadingSummit2::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Section background card
    p.setPen(Qt::NoPen);
    p.setBrush(CLR_CARD);
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 8, 8);

    p.setPen(CLR_TEXT);
    p.setFont(QFont("Sans", 10, QFont::Bold));
    p.drawText(rect.adjusted(14, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop,
               "Approach Distribution");

    // Count approaches
    QMap<QString, int> approachCounts;
    for (const auto& e : entries_) approachCounts[e.approach]++;
    int total = entries_.size();

    if (total == 0) {
        p.setPen(CLR_MUTED);
        p.setFont(QFont("Sans", 9));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    QList<QColor> palette = {CLR_BLUE, CLR_GREEN, CLR_AMBER, CLR_RED, CLR_PURPLE};

    // Donut chart
    int cx = rect.left() + rect.width() / 2;
    int cy = rect.top() + 32 + (rect.height() - 80) / 2;
    int outerR = qMin(rect.width(), rect.height() - 90) / 2 - 20;
    int innerR = outerR * 55 / 100;
    if (outerR < 30) outerR = 30;

    qreal startAngle = 0.0;
    int ci = 0;
    auto keys = approachCounts.keys();
    for (const auto& approach : keys) {
        int count = approachCounts[approach];
        qreal span = 360.0 * count / total;

        QPainterPath slice;
        QRectF outerRect(cx - outerR, cy - outerR, outerR * 2, outerR * 2);
        QRectF innerRect(cx - innerR, cy - innerR, innerR * 2, innerR * 2);
        slice.arcMoveTo(outerRect, -startAngle * 16 / 16);
        slice.arcTo(outerRect, -startAngle, -span);
        slice.arcTo(innerRect, -(startAngle + span), span);
        slice.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(palette[ci % palette.size()]);
        p.drawPath(slice);

        // Label line
        qreal midAngle = qDegreesToRadians(startAngle + span / 2.0);
        int labelR = outerR + 12;
        int lx = cx + static_cast<int>(labelR * qCos(midAngle));
        int ly = cy - static_cast<int>(labelR * qSin(midAngle));
        p.setPen(CLR_TEXT);
        p.setFont(QFont("Sans", 7));
        p.drawText(lx - 20, ly - 4, 40, 14, Qt::AlignCenter,
                   QString("%1\n%2").arg(approach).arg(count));

        startAngle += span;
        ++ci;
    }

    // Center text
    p.setPen(CLR_TEXT);
    p.setFont(QFont("Sans", 14, QFont::Bold));
    p.drawText(QRect(cx - 20, cy - 12, 40, 24),
               Qt::AlignCenter, QString::number(total));
    p.setFont(QFont("Sans", 7));
    p.setPen(CLR_MUTED);
    p.drawText(QRect(cx - 20, cy + 10, 40, 14),
               Qt::AlignCenter, "total");
}

void PaperReadingSummit2::drawStats(QPainter& p, const QRect& rect) {
    // Background
    p.setPen(Qt::NoPen);
    p.setBrush(CLR_CARD);
    p.drawRoundedRect(rect.adjusted(4, 2, -4, -2), 8, 8);

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    int totalStages = 0;
    for (const auto& e : entries_) totalStages += e.stages;

    QList<Stat> stats = {
        {"Total Peaks",   QString::number(entries_.size()), CLR_BLUE},
        {"Summited",      QString::number(summitedCount()), CLR_GREEN},
        {"Avg Altitude",  QString::number(avgAltitude(), 'f', 0),     CLR_AMBER},
        {"Total Stages",  QString::number(totalStages),               CLR_PURPLE}
    };

    int boxCount = stats.size();
    int gap = 8;
    int totalGap = gap * (boxCount + 1);
    int boxW = (rect.width() - totalGap) / boxCount;
    int boxH = rect.height() - 12;
    if (boxH > 56) boxH = 56;

    for (int i = 0; i < boxCount; ++i) {
        int bx = rect.left() + gap + i * (boxW + gap);
        int by = rect.top() + (rect.height() - boxH) / 2;

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 6, 6);

        // Top color accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, by, boxW, 4, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Sans", 16, QFont::Bold));
        p.drawText(QRect(bx, by + 6, boxW, 28),
                   Qt::AlignCenter, stats[i].value);

        // Label
        p.setPen(CLR_MUTED);
        p.setFont(QFont("Sans", 8));
        p.drawText(QRect(bx, by + 34, boxW, 16),
                   Qt::AlignCenter, stats[i].label);
    }
}

void PaperReadingSummit2::updateInfo() {
    infoLabel_->setText(
        QString("Peaks: %1 | Summited: %2 | Avg Alt: %3 | Stages: %4")
            .arg(entries_.size())
            .arg(summitedCount())
            .arg(QString::number(avgAltitude(), 'f', 1))
            .arg(entries_.isEmpty() ? 0 : [this]() {
                int t = 0;
                for (const auto& e : entries_) t += e.stages;
                return t;
            }()));
}

void PaperReadingSummit2::loadSettings() {
    settings_.beginGroup("Summit2Entries");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        ReadingSummit2Entry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.peak = settings_.value(QString("peak_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.approach = settings_.value(QString("approach_%1").arg(i)).toString();
        e.altitude = settings_.value(QString("altitude_%1").arg(i)).toDouble();
        e.stages = settings_.value(QString("stages_%1").arg(i)).toInt();
        e.summited = settings_.value(QString("summited_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();
    updateInfo();
}

void PaperReadingSummit2::saveSettings() {
    settings_.beginGroup("Summit2Entries");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("peak_%1").arg(i), e.peak);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("approach_%1").arg(i), e.approach);
        settings_.setValue(QString("altitude_%1").arg(i), e.altitude);
        settings_.setValue(QString("stages_%1").arg(i), e.stages);
        settings_.setValue(QString("summited_%1").arg(i), e.summited);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
