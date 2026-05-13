#include "analysis/PaperCitationAudit.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPainterPath>
#include <QtMath>

PaperCitationAudit::PaperCitationAudit(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationAudit")
    , auditBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperCitationAudit::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(8);

    // Top control bar
    auto* controlLayout = new QHBoxLayout;
    controlLayout->setSpacing(8);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter citation reference (e.g. Smith et al., 2023)"));
    inputField_->setMinimumWidth(320);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Journal"),     QStringLiteral("Journal"));
    categoryCombo_->addItem(tr("Conference"),  QStringLiteral("Conference"));
    categoryCombo_->addItem(tr("Book"),        QStringLiteral("Book"));
    categoryCombo_->addItem(tr("Preprint"),    QStringLiteral("Preprint"));
    categoryCombo_->addItem(tr("Website"),     QStringLiteral("Website"));
    categoryCombo_->setMinimumWidth(120);

    auditBtn_ = new QPushButton(tr("Audit"), this);
    auditBtn_->setFixedWidth(80);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setFixedWidth(70);

    controlLayout->addWidget(inputField_);
    controlLayout->addWidget(categoryCombo_);
    controlLayout->addWidget(auditBtn_);
    controlLayout->addWidget(clearBtn_);
    controlLayout->addStretch();

    rootLayout->addLayout(controlLayout);

    // Info label
    infoLabel_ = new QLabel(tr("No citations audited yet."), this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px; padding: 4px 0;"));
    rootLayout->addWidget(infoLabel_);

    // Stretch for paint area
    rootLayout->addStretch(1);

    setMinimumSize(860, 520);

    // Connections
    connect(auditBtn_,   &QPushButton::clicked, this, &PaperCitationAudit::onAudit);
    connect(clearBtn_,   &QPushButton::clicked, this, &PaperCitationAudit::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperCitationAudit::onAudit);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PaperCitationAudit::addEntry(const CitationAuditEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    update();
    saveSettings();
}

QList<CitationAuditEntry> PaperCitationAudit::entries() const
{
    return entries_;
}

int PaperCitationAudit::verifiedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.verified)
            ++count;
    }
    return count;
}

qreal PaperCitationAudit::avgAccuracy() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.accuracy;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperCitationAudit::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperCitationAudit::onAudit()
{
    QString refText = inputField_->text().trimmed();
    if (refText.isEmpty())
        return;

    static int nextId = 1;

    const QStringList statuses = {
        QStringLiteral("Verified"), QStringLiteral("Unverified"),
        QStringLiteral("Disputed"), QStringLiteral("Retracted"),
        QStringLiteral("Pending")
    };

    QRandomGenerator* rng = QRandomGenerator::global();

    qreal accuracy = 50.0 + rng->bounded(500) / 10.0; // 50.0 - 100.0
    if (accuracy > 100.0) accuracy = 100.0;
    int checks = 1 + rng->bounded(10);
    bool verified = accuracy >= 75.0;

    const QColor palette[] = {
        QColor(QStringLiteral("#3b82f6")),
        QColor(QStringLiteral("#16a34a")),
        QColor(QStringLiteral("#d97706")),
        QColor(QStringLiteral("#dc2626")),
        QColor(QStringLiteral("#7c3aed"))
    };
    QColor color = palette[nextId % 5];

    CitationAuditEntry entry;
    entry.id        = nextId++;
    entry.reference = refText;
    entry.category  = categoryCombo_->currentData().toString();
    entry.status    = verified ? QStringLiteral("Verified")
                               : statuses.at(rng->bounded(statuses.size()));
    entry.accuracy  = accuracy;
    entry.checks    = checks;
    entry.verified  = verified;
    entry.color     = color;

    entries_.append(entry);

    inputField_->clear();
    updateInfo();
    update();
    saveSettings();

    emit citationAudited(entry.id, entry.accuracy);
}

void PaperCitationAudit::onClear()
{
    entries_.clear();
    updateInfo();
    update();
    saveSettings();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperCitationAudit::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int controlHeight = 80; // approximate height of controls + info
    QRect paintRect(0, controlHeight, width(), height() - controlHeight);

    // Background
    p.fillRect(paintRect, QColor(250, 250, 252));

    // Split: left 55% for audit list, right 45% for charts
    int leftW   = static_cast<int>(paintRect.width() * 0.55);
    int rightW  = paintRect.width() - leftW;
    int rightH  = paintRect.height() / 2;

    QRect listRect(paintRect.x(), paintRect.y(), leftW, paintRect.height());
    QRect chartRect(paintRect.x() + leftW, paintRect.y(), rightW, rightH);
    QRect statsRect(paintRect.x() + leftW, paintRect.y() + rightH, rightW, paintRect.height() - rightH);

    drawAuditView(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperCitationAudit::drawAuditView(QPainter& p, const QRect& rect)
{
    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 10, 10);

    // Title
    p.setPen(QColor(31, 41, 55));
    QFont titleFont = font();
    titleFont.setPointSize(12);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(16, 14, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Citation Audit (%1)").arg(entries_.size()));

    QFont itemFont = font();
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    int y = rect.y() + 42;
    const int lineH = 52;
    const int bottomLimit = rect.bottom() - 8;

    for (int i = 0; i < entries_.size() && y + lineH < bottomLimit; ++i) {
        const auto& e = entries_[i];

        // Color indicator bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + 16, y, 4, lineH - 8, 2, 2);

        // Reference text (truncated)
        p.setPen(QColor(31, 41, 55));
        QString display = e.reference;
        if (display.length() > 42)
            display = display.left(39) + QStringLiteral("...");
        p.drawText(rect.x() + 28, y + 14, display);

        // Meta line: category, checks count, status
        QString meta = QStringLiteral("%1  |  Checks: %2  |  %3")
                           .arg(e.category)
                           .arg(e.checks)
                           .arg(e.status);
        p.setPen(QColor(107, 114, 128));
        p.drawText(rect.x() + 28, y + 32, meta);

        // Accuracy badge
        int accInt = static_cast<int>(e.accuracy);
        if (e.verified) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(22, 163, 74, 40));
            p.drawRoundedRect(rect.right() - 100, y + 4, 78, 20, 4, 4);
            p.setPen(QColor(22, 163, 74));
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(220, 38, 38, 40));
            p.drawRoundedRect(rect.right() - 100, y + 4, 78, 20, 4, 4);
            p.setPen(QColor(220, 38, 38));
        }
        QFont badgeFont = itemFont;
        badgeFont.setPointSize(8);
        badgeFont.setBold(true);
        p.setFont(badgeFont);
        QString badge = e.verified ? tr("Verified %1%").arg(accInt)
                                   : tr("Audit %1%").arg(accInt);
        p.drawText(QRect(rect.right() - 100, y + 4, 78, 20),
                   Qt::AlignCenter, badge);

        // Checks indicator
        p.setPen(QColor(59, 130, 246));
        QFont smallFont = itemFont;
        smallFont.setPointSize(8);
        p.setFont(smallFont);
        p.drawText(rect.right() - 100, y + 36, tr("%n check(s)", nullptr, e.checks));

        p.setFont(itemFont);
        y += lineH;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor(156, 163, 175));
        QFont emptyFont = font();
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.drawText(rect, Qt::AlignCenter, tr("No citations yet.\nEnter text and click Audit."));
    }
}

void PaperCitationAudit::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 10, 10);

    // Title
    p.setPen(QColor(31, 41, 55));
    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(16, 12, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Categories"));

    auto counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor(156, 163, 175));
        QFont emptyFont = font();
        emptyFont.setPointSize(9);
        p.setFont(emptyFont);
        p.drawText(rect, Qt::AlignCenter, tr("No data"));
        return;
    }

    const QColor palette[] = {
        QColor(QStringLiteral("#3b82f6")),
        QColor(QStringLiteral("#16a34a")),
        QColor(QStringLiteral("#d97706")),
        QColor(QStringLiteral("#dc2626")),
        QColor(QStringLiteral("#7c3aed"))
    };

    int total = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        total += it.value();

    // Draw horizontal bar chart
    QFont barFont = font();
    barFont.setPointSize(9);
    p.setFont(barFont);

    int y = rect.y() + 40;
    const int barH = 22;
    const int barSpacing = 8;
    const int labelW = 80;
    const int bottomLimit = rect.bottom() - 16;
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd() && y + barH < bottomLimit; ++it) {
        qreal ratio = static_cast<qreal>(it.value()) / static_cast<qreal>(total);
        int maxBarW = rect.width() - labelW - 80;

        // Category label
        p.setPen(QColor(75, 85, 99));
        p.drawText(rect.x() + 16, y + barH - 6, it.key());

        // Bar background
        int barX = rect.x() + labelW;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(243, 244, 246));
        p.drawRoundedRect(barX, y + 2, maxBarW, barH - 4, 4, 4);

        // Bar fill
        QColor fillColor = palette[colorIdx % 5];
        fillColor.setAlpha(200);
        p.setBrush(fillColor);
        int fillW = static_cast<int>(maxBarW * ratio);
        if (fillW > 0)
            p.drawRoundedRect(barX, y + 2, fillW, barH - 4, 4, 4);

        // Count label
        p.setPen(QColor(31, 41, 55));
        p.drawText(barX + maxBarW + 8, y + barH - 6,
                   QStringLiteral("%1").arg(it.value()));

        y += barH + barSpacing;
        ++colorIdx;
    }
}

void PaperCitationAudit::drawStats(QPainter& p, const QRect& rect)
{
    // Background card
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(255, 255, 255));
    p.drawRoundedRect(rect.adjusted(4, 4, -4, -4), 10, 10);

    // Title
    p.setPen(QColor(31, 41, 55));
    QFont titleFont = font();
    titleFont.setPointSize(11);
    titleFont.setBold(true);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(16, 12, -16, 0), Qt::AlignLeft | Qt::AlignTop,
               tr("Statistics"));

    QFont statFont = font();
    statFont.setPointSize(10);
    p.setFont(statFont);

    int y = rect.y() + 42;
    const int lineH = 28;

    // Total entries
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Total:"));
    p.setPen(QColor(31, 41, 55));
    p.drawText(rect.x() + 130, y, QStringLiteral("%1").arg(entries_.size()));
    y += lineH;

    // Verified count
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Verified:"));
    int vc = verifiedCount();
    p.setPen(vc > 0 ? QColor(22, 163, 74) : QColor(107, 114, 128));
    p.drawText(rect.x() + 130, y, QStringLiteral("%1").arg(vc));
    y += lineH;

    // Unverified count
    int unverifiedCount = entries_.size() - vc;
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Unverified:"));
    p.setPen(unverifiedCount > 0 ? QColor(220, 38, 38) : QColor(107, 114, 128));
    p.drawText(rect.x() + 130, y, QStringLiteral("%1").arg(unverifiedCount));
    y += lineH;

    // Average accuracy
    qreal avg = avgAccuracy();
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Avg Accuracy:"));
    if (avg >= 75.0)
        p.setPen(QColor(22, 163, 74));
    else if (avg >= 50.0)
        p.setPen(QColor(217, 119, 6));
    else
        p.setPen(QColor(220, 38, 38));
    p.drawText(rect.x() + 130, y, QStringLiteral("%1%").arg(qRound(avg)));
    y += lineH;

    // Total checks
    int totalChecks = 0;
    for (const auto& e : entries_)
        totalChecks += e.checks;
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Total Checks:"));
    p.setPen(QColor(59, 130, 246));
    p.drawText(rect.x() + 130, y, QStringLiteral("%1").arg(totalChecks));
    y += lineH + 4;

    // Accuracy bar visual
    if (!entries_.isEmpty()) {
        int barX = rect.x() + 20;
        int barW = rect.width() - 40;
        int barH = 12;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(229, 231, 235));
        p.drawRoundedRect(barX, y, barW, barH, 6, 6);

        // Gradient fill based on average accuracy
        QColor barColor;
        if (avg >= 75.0)
            barColor = QColor(QStringLiteral("#16a34a"));
        else if (avg >= 50.0)
            barColor = QColor(QStringLiteral("#d97706"));
        else
            barColor = QColor(QStringLiteral("#dc2626"));

        qreal ratio = qBound(0.0, avg / 100.0, 1.0);
        int fillW = static_cast<int>(barW * ratio);
        if (fillW > 0) {
            p.setBrush(barColor);
            p.drawRoundedRect(barX, y, fillW, barH, 6, 6);
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void PaperCitationAudit::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No citations audited yet."));
        return;
    }

    int vc = verifiedCount();
    qreal avg = avgAccuracy();
    infoLabel_->setText(tr("%1 citation(s) | %2 verified | Avg accuracy: %3%")
                            .arg(entries_.size())
                            .arg(vc)
                            .arg(qRound(avg)));
}

void PaperCitationAudit::loadSettings()
{
    int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationAuditEntry e;
        e.id        = settings_.value(QStringLiteral("id")).toInt();
        e.reference = settings_.value(QStringLiteral("reference")).toString();
        e.category  = settings_.value(QStringLiteral("category")).toString();
        e.status    = settings_.value(QStringLiteral("status")).toString();
        e.accuracy  = settings_.value(QStringLiteral("accuracy")).toReal();
        e.checks    = settings_.value(QStringLiteral("checks")).toInt();
        e.verified  = settings_.value(QStringLiteral("verified")).toBool();
        e.color     = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();

    // Seed 8 entries if empty
    if (entries_.isEmpty()) {
        const QStringList refs = {
            QStringLiteral("Smith et al., 2023, Nature"),
            QStringLiteral("Johnson & Lee, 2022, Science"),
            QStringLiteral("Wang et al., 2024, ICML"),
            QStringLiteral("Brown, 2021, ML Handbook"),
            QStringLiteral("Davis et al., 2023, arXiv:2301.04234"),
            QStringLiteral("Garcia & Martinez, 2020, ACL"),
            QStringLiteral("Miller et al., 2024, JMLR"),
            QStringLiteral("Wilson, 2023, deepai.org/paper")
        };

        const QStringList categories = {
            QStringLiteral("Journal"),
            QStringLiteral("Conference"),
            QStringLiteral("Book"),
            QStringLiteral("Preprint"),
            QStringLiteral("Website")
        };

        const QStringList statuses = {
            QStringLiteral("Verified"), QStringLiteral("Unverified"),
            QStringLiteral("Disputed"), QStringLiteral("Pending")
        };

        const QColor palette[] = {
            QColor(QStringLiteral("#3b82f6")),
            QColor(QStringLiteral("#16a34a")),
            QColor(QStringLiteral("#d97706")),
            QColor(QStringLiteral("#dc2626")),
            QColor(QStringLiteral("#7c3aed"))
        };

        QRandomGenerator* rng = QRandomGenerator::global();

        for (int i = 0; i < 8; ++i) {
            qreal accuracy = 50.0 + rng->bounded(500) / 10.0;
            if (accuracy > 100.0) accuracy = 100.0;
            int checks = 1 + rng->bounded(10);
            bool verified = accuracy >= 75.0;

            CitationAuditEntry e;
            e.id        = i + 1;
            e.reference = refs[i];
            e.category  = categories[i % categories.size()];
            e.status    = verified ? QStringLiteral("Verified")
                                   : statuses.at(rng->bounded(statuses.size()));
            e.accuracy  = accuracy;
            e.checks    = checks;
            e.verified  = verified;
            e.color     = palette[i % 5];
            entries_.append(e);
        }

        saveSettings();
    }

    updateInfo();
}

void PaperCitationAudit::saveSettings()
{
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"),        e.id);
        settings_.setValue(QStringLiteral("reference"), e.reference);
        settings_.setValue(QStringLiteral("category"),  e.category);
        settings_.setValue(QStringLiteral("status"),    e.status);
        settings_.setValue(QStringLiteral("accuracy"),  e.accuracy);
        settings_.setValue(QStringLiteral("checks"),    e.checks);
        settings_.setValue(QStringLiteral("verified"),  e.verified);
        settings_.setValue(QStringLiteral("color"),     e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
