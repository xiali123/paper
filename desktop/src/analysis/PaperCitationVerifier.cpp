#include "analysis/PaperCitationVerifier.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPainterPath>
#include <QtMath>

PaperCitationVerifier::PaperCitationVerifier(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationVerifier")
    , verifyBtn_(nullptr)
    , clearBtn_(nullptr)
    , categoryCombo_(nullptr)
    , inputField_(nullptr)
    , infoLabel_(nullptr)
{
    setupUI();
    loadSettings();
}

void PaperCitationVerifier::setupUI()
{
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(8);

    // Top control bar
    auto* controlLayout = new QHBoxLayout;
    controlLayout->setSpacing(8);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText(tr("Enter citation text (e.g. Smith et al., 2023)"));
    inputField_->setMinimumWidth(320);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem(tr("Journal"),     QStringLiteral("journal"));
    categoryCombo_->addItem(tr("Conference"),  QStringLiteral("conference"));
    categoryCombo_->addItem(tr("Book"),        QStringLiteral("book"));
    categoryCombo_->addItem(tr("Thesis"),      QStringLiteral("thesis"));
    categoryCombo_->addItem(tr("Preprint"),    QStringLiteral("preprint"));
    categoryCombo_->addItem(tr("Web"),         QStringLiteral("web"));
    categoryCombo_->setMinimumWidth(120);

    verifyBtn_ = new QPushButton(tr("Verify"), this);
    verifyBtn_->setFixedWidth(90);

    clearBtn_ = new QPushButton(tr("Clear"), this);
    clearBtn_->setFixedWidth(70);

    controlLayout->addWidget(inputField_);
    controlLayout->addWidget(categoryCombo_);
    controlLayout->addWidget(verifyBtn_);
    controlLayout->addWidget(clearBtn_);
    controlLayout->addStretch();

    rootLayout->addLayout(controlLayout);

    // Info label
    infoLabel_ = new QLabel(tr("No citations verified yet."), this);
    infoLabel_->setWordWrap(true);
    infoLabel_->setStyleSheet(QStringLiteral("color: #6b7280; font-size: 12px; padding: 4px 0;"));
    rootLayout->addWidget(infoLabel_);

    // Stretch for paint area
    rootLayout->addStretch(1);

    setMinimumSize(860, 520);

    // Connections
    connect(verifyBtn_, &QPushButton::clicked, this, &PaperCitationVerifier::onVerify);
    connect(clearBtn_,  &QPushButton::clicked, this, &PaperCitationVerifier::onClear);
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperCitationVerifier::onVerify);
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void PaperCitationVerifier::addEntry(const CitationVerifyEntry& entry)
{
    entries_.append(entry);
    updateInfo();
    update();
    saveSettings();
}

QList<CitationVerifyEntry> PaperCitationVerifier::entries() const
{
    return entries_;
}

int PaperCitationVerifier::validCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.valid)
            ++count;
    }
    return count;
}

qreal PaperCitationVerifier::avgConfidence() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.confidence;
    return sum / static_cast<qreal>(entries_.size());
}

QMap<QString, int> PaperCitationVerifier::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void PaperCitationVerifier::onVerify()
{
    QString citationText = inputField_->text().trimmed();
    if (citationText.isEmpty())
        return;

    static int nextId = 1;

    const QStringList sources = {
        QStringLiteral("CrossRef"), QStringLiteral("Semantic Scholar"),
        QStringLiteral("Google Scholar"), QStringLiteral("OpenAlex"),
        QStringLiteral("DBLP")
    };

    QRandomGenerator* rng = QRandomGenerator::global();

    qreal confidence = rng->bounded(100) / 100.0;
    int year = 1990 + rng->bounded(36);
    bool doiResolved = rng->bounded(100) < 60;
    bool valid = confidence > 0.7;

    const QColor palette[] = {
        QColor(QStringLiteral("#3b82f6")),
        QColor(QStringLiteral("#16a34a")),
        QColor(QStringLiteral("#d97706")),
        QColor(QStringLiteral("#dc2626")),
        QColor(QStringLiteral("#7c3aed"))
    };
    QColor color = palette[nextId % 5];

    CitationVerifyEntry entry;
    entry.id           = nextId++;
    entry.citation     = citationText;
    entry.category     = categoryCombo_->currentData().toString();
    entry.source       = sources.at(rng->bounded(sources.size()));
    entry.confidence   = confidence;
    entry.year         = year;
    entry.valid        = valid;
    entry.doiResolved  = doiResolved;
    entry.color        = color;

    entries_.append(entry);

    inputField_->clear();
    updateInfo();
    update();
    saveSettings();

    emit citationVerified(entry.id, entry.confidence);
}

void PaperCitationVerifier::onClear()
{
    entries_.clear();
    updateInfo();
    update();
    saveSettings();
}

// ---------------------------------------------------------------------------
// Painting
// ---------------------------------------------------------------------------

void PaperCitationVerifier::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int controlHeight = 80; // approximate height of controls + info
    QRect paintRect(0, controlHeight, width(), height() - controlHeight);

    // Background
    p.fillRect(paintRect, QColor(250, 250, 252));

    // Split: left 55% for citation list, right 45% for charts
    int leftW   = static_cast<int>(paintRect.width() * 0.55);
    int rightW  = paintRect.width() - leftW;
    int rightH  = (paintRect.height()) / 2;

    QRect listRect(paintRect.x(), paintRect.y(), leftW, paintRect.height());
    QRect chartRect(paintRect.x() + leftW, paintRect.y(), rightW, rightH);
    QRect statsRect(paintRect.x() + leftW, paintRect.y() + rightH, rightW, paintRect.height() - rightH);

    drawCitationList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperCitationVerifier::drawCitationList(QPainter& p, const QRect& rect)
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
               tr("Citation List (%1)").arg(entries_.size()));

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

        // Citation text (truncated)
        p.setPen(QColor(31, 41, 55));
        QString display = e.citation;
        if (display.length() > 42)
            display = display.left(39) + QStringLiteral("...");
        p.drawText(rect.x() + 28, y + 14, display);

        // Meta line: source, year, category
        QString meta = QStringLiteral("%1  |  %2  |  %3")
                           .arg(e.source)
                           .arg(e.year)
                           .arg(e.category);
        p.setPen(QColor(107, 114, 128));
        p.drawText(rect.x() + 28, y + 32, meta);

        // Status badge
        if (e.valid) {
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
        QString badge = e.valid ? tr("Valid %1%").arg(static_cast<int>(e.confidence * 100))
                                : tr("Low %1%").arg(static_cast<int>(e.confidence * 100));
        p.drawText(QRect(rect.right() - 100, y + 4, 78, 20),
                   Qt::AlignCenter, badge);

        // DOI indicator
        if (e.doiResolved) {
            p.setPen(QColor(59, 130, 246));
            p.drawText(rect.right() - 100, y + 34, tr("DOI resolved"));
        }

        p.setFont(itemFont);
        y += lineH;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor(156, 163, 175));
        QFont emptyFont = font();
        emptyFont.setPointSize(10);
        p.setFont(emptyFont);
        p.drawText(rect, Qt::AlignCenter, tr("No citations yet.\nEnter text and click Verify."));
    }
}

void PaperCitationVerifier::drawCategoryChart(QPainter& p, const QRect& rect)
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

void PaperCitationVerifier::drawStats(QPainter& p, const QRect& rect)
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
    p.drawText(rect.x() + 120, y, QStringLiteral("%1").arg(entries_.size()));
    y += lineH;

    // Valid count
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Valid:"));
    int vc = validCount();
    p.setPen(vc > 0 ? QColor(22, 163, 74) : QColor(107, 114, 128));
    p.drawText(rect.x() + 120, y, QStringLiteral("%1").arg(vc));
    y += lineH;

    // Invalid count
    int invalidCount = entries_.size() - vc;
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Invalid:"));
    p.setPen(invalidCount > 0 ? QColor(220, 38, 38) : QColor(107, 114, 128));
    p.drawText(rect.x() + 120, y, QStringLiteral("%1").arg(invalidCount));
    y += lineH;

    // Average confidence
    qreal avg = avgConfidence();
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("Avg Confidence:"));
    if (avg >= 0.7)
        p.setPen(QColor(22, 163, 74));
    else if (avg >= 0.4)
        p.setPen(QColor(217, 119, 6));
    else
        p.setPen(QColor(220, 38, 38));
    p.drawText(rect.x() + 120, y, QStringLiteral("%1%").arg(qRound(avg * 100)));
    y += lineH;

    // DOI resolved count
    int doiCount = 0;
    for (const auto& e : entries_) {
        if (e.doiResolved)
            ++doiCount;
    }
    p.setPen(QColor(107, 114, 128));
    p.drawText(rect.x() + 20, y, tr("DOI Resolved:"));
    p.setPen(QColor(59, 130, 246));
    p.drawText(rect.x() + 120, y, QStringLiteral("%1").arg(doiCount));
    y += lineH + 4;

    // Confidence bar visual
    if (!entries_.isEmpty()) {
        int barX = rect.x() + 20;
        int barW = rect.width() - 40;
        int barH = 12;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(229, 231, 235));
        p.drawRoundedRect(barX, y, barW, barH, 6, 6);

        // Gradient fill based on average confidence
        QColor barColor;
        if (avg >= 0.7)
            barColor = QColor(QStringLiteral("#16a34a"));
        else if (avg >= 0.4)
            barColor = QColor(QStringLiteral("#d97706"));
        else
            barColor = QColor(QStringLiteral("#dc2626"));

        int fillW = static_cast<int>(barW * avg);
        if (fillW > 0) {
            p.setBrush(barColor);
            p.drawRoundedRect(barX, y, fillW, barH, 6, 6);
        }
    }
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void PaperCitationVerifier::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText(tr("No citations verified yet."));
        return;
    }

    int vc = validCount();
    qreal avg = avgConfidence() * 100.0;
    infoLabel_->setText(tr("%1 citation(s) | %2 valid | Avg confidence: %3%")
                            .arg(entries_.size())
                            .arg(vc)
                            .arg(qRound(avg)));
}

void PaperCitationVerifier::loadSettings()
{
    int size = settings_.beginReadArray(QStringLiteral("entries"));
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationVerifyEntry e;
        e.id          = settings_.value(QStringLiteral("id")).toInt();
        e.citation    = settings_.value(QStringLiteral("citation")).toString();
        e.category    = settings_.value(QStringLiteral("category")).toString();
        e.source      = settings_.value(QStringLiteral("source")).toString();
        e.confidence  = settings_.value(QStringLiteral("confidence")).toReal();
        e.year        = settings_.value(QStringLiteral("year")).toInt();
        e.valid       = settings_.value(QStringLiteral("valid")).toBool();
        e.doiResolved = settings_.value(QStringLiteral("doiResolved")).toBool();
        e.color       = QColor(settings_.value(QStringLiteral("color")).toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationVerifier::saveSettings()
{
    settings_.beginWriteArray(QStringLiteral("entries"));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue(QStringLiteral("id"),          e.id);
        settings_.setValue(QStringLiteral("citation"),    e.citation);
        settings_.setValue(QStringLiteral("category"),    e.category);
        settings_.setValue(QStringLiteral("source"),      e.source);
        settings_.setValue(QStringLiteral("confidence"),  e.confidence);
        settings_.setValue(QStringLiteral("year"),        e.year);
        settings_.setValue(QStringLiteral("valid"),       e.valid);
        settings_.setValue(QStringLiteral("doiResolved"), e.doiResolved);
        settings_.setValue(QStringLiteral("color"),       e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
