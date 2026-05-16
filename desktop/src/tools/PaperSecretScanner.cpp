#include "tools/PaperSecretScanner.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QPaintEvent>
#include <QFontMetrics>
#include <QtMath>

PaperSecretScanner::PaperSecretScanner(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "SecretScanner")
{
    setupUI();
    loadSettings();
}

void PaperSecretScanner::setupUI()
{
    auto* mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // --- Left: controls ---
    auto* leftLayout = new QVBoxLayout;
    leftLayout->setSpacing(4);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All Categories", "API Keys", "Passwords", "Tokens",
                               "Certificates", "Private Keys", "Credentials"});
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("File path...");
    leftLayout->addWidget(inputField_);

    auto* btnRow = new QHBoxLayout;
    btnRow->setSpacing(4);

    scanBtn_ = new QPushButton("Scan", this);
    clearBtn_ = new QPushButton("Clear", this);
    btnRow->addWidget(scanBtn_);
    btnRow->addWidget(clearBtn_);
    leftLayout->addLayout(btnRow);

    infoLabel_ = new QLabel("Findings: 0 | Critical: 0 | Avg Confidence: 0.0%", this);
    leftLayout->addWidget(infoLabel_);

    leftLayout->addStretch();

    mainLayout->addLayout(leftLayout);

    // --- Right: stretch for painted area ---
    mainLayout->addStretch(1);

    // Connections
    connect(scanBtn_, &QPushButton::clicked, this, &PaperSecretScanner::onScan);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSecretScanner::onClear);
}

void PaperSecretScanner::loadSettings()
{
    settings_.beginGroup("SecretScanner");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        SecretEntry e;
        e.id          = settings_.value("id").toInt();
        e.file        = settings_.value("file").toString();
        e.category    = settings_.value("category").toString();
        e.severity    = settings_.value("severity").toString();
        e.confidence  = settings_.value("confidence").toReal();
        e.occurrences = settings_.value("occurrences").toInt();
        e.critical    = settings_.value("critical").toBool();
        e.color       = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperSecretScanner::saveSettings()
{
    settings_.beginGroup("SecretScanner");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id",          e.id);
        settings_.setValue("file",        e.file);
        settings_.setValue("category",    e.category);
        settings_.setValue("severity",    e.severity);
        settings_.setValue("confidence",  e.confidence);
        settings_.setValue("occurrences", e.occurrences);
        settings_.setValue("critical",    e.critical);
        settings_.setValue("color",       e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

// ── Public helpers ──────────────────────────────────────────────────────────

void PaperSecretScanner::addEntry(const SecretEntry& entry)
{
    entries_.append(entry);
}

QList<SecretEntry> PaperSecretScanner::entries() const
{
    return entries_;
}

int PaperSecretScanner::criticalCount() const
{
    int n = 0;
    for (const auto& e : entries_)
        if (e.critical) ++n;
    return n;
}

qreal PaperSecretScanner::avgConfidence() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0;
    for (const auto& e : entries_)
        sum += e.confidence;
    return sum / entries_.size();
}

QMap<QString, int> PaperSecretScanner::categoryCounts() const
{
    QMap<QString, int> m;
    for (const auto& e : entries_)
        m[e.category]++;
    return m;
}

// ── Slots ───────────────────────────────────────────────────────────────────

void PaperSecretScanner::onScan()
{
    const QString filePath = inputField_->text().trimmed();
    if (filePath.isEmpty()) return;

    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    const qreal confidence = QRandomGenerator::global()->bounded(50, 101) / 100.0;
    const int occurrences  = QRandomGenerator::global()->bounded(1, 21);
    const bool critical    = confidence > 0.8;

    static const QStringList severities = {"high", "medium", "low"};
    const QString severity = severities.at(
        QRandomGenerator::global()->bounded(0, severities.size()));

    const QString category = categoryCombo_->currentText() == "All Categories"
        ? QStringList{"API Keys","Passwords","Tokens","Certificates","Private Keys","Credentials"}
          .at(QRandomGenerator::global()->bounded(0, 6))
        : categoryCombo_->currentText();

    QColor color;
    if (critical) {
        color = QColor("#dc2626");
    } else if (severity == "high") {
        color = QColor("#d97706");
    } else if (severity == "low") {
        color = QColor("#16a34a");
    } else {
        color = palette.at(QRandomGenerator::global()->bounded(0, palette.size()));
    }

    SecretEntry entry;
    entry.id          = entries_.size() + 1;
    entry.file        = filePath;
    entry.category    = category;
    entry.severity    = severity;
    entry.confidence  = confidence;
    entry.occurrences = occurrences;
    entry.critical    = critical;
    entry.color       = color;

    addEntry(entry);
    saveSettings();
    updateInfo();
    update();
    emit secretFound(entry.id, entry.confidence);
}

void PaperSecretScanner::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

// ── Painting ────────────────────────────────────────────────────────────────

void PaperSecretScanner::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const int w = width();
    const int h = height();
    const int colW = w / 3;

    drawSecretList(p, QRect(0, 0, colW, h));
    drawCategoryChart(p, QRect(colW, 0, colW, h));
    drawStats(p, QRect(colW * 2, 0, w - colW * 2, h));
}

void PaperSecretScanner::drawSecretList(QPainter& p, const QRect& rect)
{
    // Background
    p.fillRect(rect, QColor("#1e293b"));

    // Title
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Secret Scanner");

    // Entries
    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    const int topMargin = 30;
    const int rowHeight = 42;
    const int barHeight  = 6;
    const int textAreaW  = rect.width() - 16;

    int y = rect.y() + topMargin;
    for (int i = 0; i < entries_.size() && y + rowHeight < rect.bottom(); ++i) {
        const auto& e = entries_[i];
        const int x = rect.x() + 8;

        // Row background
        QColor rowBg = (i % 2 == 0) ? QColor("#1e293b") : QColor("#273548");
        p.fillRect(QRect(x, y, textAreaW, rowHeight), rowBg);

        // File name
        p.setPen(Qt::white);
        QString label = QString("#%1 %2").arg(e.id).arg(e.file);
        p.drawText(QRect(x + 4, y + 2, textAreaW - 8, 16), Qt::AlignLeft | Qt::AlignVCenter,
                   p.fontMetrics().elidedText(label, Qt::ElideRight, textAreaW - 12));

        // Confidence bar background
        const int barY = y + 22;
        const int barW = textAreaW - 8;
        p.fillRect(QRect(x + 4, barY, barW, barHeight), QColor("#334155"));

        // Confidence bar fill
        QColor barColor;
        if (e.critical)        barColor = QColor("#dc2626");
        else if (e.severity == "high") barColor = QColor("#d97706");
        else if (e.severity == "low")  barColor = QColor("#16a34a");
        else                            barColor = e.color;

        p.fillRect(QRect(x + 4, barY, static_cast<int>(barW * e.confidence), barHeight), barColor);

        // Severity label
        p.setPen(barColor);
        QString sev = e.critical ? "CRITICAL" : e.severity.toUpper();
        p.drawText(QRect(x + 4, barY + barHeight + 1, textAreaW - 8, 12),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1  conf %2%  x%3").arg(sev)
                       .arg(qFloor(e.confidence * 100)).arg(e.occurrences));

        y += rowHeight;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#94a3b8"));
        itemFont.setItalic(true);
        p.setFont(itemFont);
        p.drawText(rect.adjusted(8, topMargin, -8, 0), Qt::AlignHCenter | Qt::AlignTop,
                   "No secrets scanned yet.");
    }
}

void PaperSecretScanner::drawCategoryChart(QPainter& p, const QRect& rect)
{
    p.fillRect(rect, QColor("#1e293b"));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

    const QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) return;

    static const QVector<QColor> catColors = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int maxVal = 0;
    for (auto it = counts.cbegin(); it != counts.cend(); ++it)
        maxVal = qMax(maxVal, it.value());

    QFont itemFont = p.font();
    itemFont.setBold(false);
    itemFont.setPointSize(9);
    p.setFont(itemFont);

    const int topMargin  = 32;
    const int leftMargin = 8;
    const int barH = 18;
    const int gap  = 6;
    const int maxBarW = rect.width() - leftMargin * 2 - 80;

    int y = rect.y() + topMargin;
    int ci = 0;
    for (auto it = counts.cbegin(); it != counts.cend() && y + barH < rect.bottom(); ++it) {
        const int x = rect.x() + leftMargin;

        // Category label
        p.setPen(Qt::white);
        p.drawText(QRect(x, y, 70, barH), Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar
        const int bw = maxVal > 0 ? static_cast<int>(maxBarW * it.value() / maxVal) : 0;
        QColor c = catColors.at(ci % catColors.size());
        p.fillRect(QRect(x + 74, y + 2, bw, barH - 4), c);

        // Count
        p.setPen(c);
        p.drawText(QRect(x + 74 + bw + 4, y, 30, barH), Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(it.value()));

        y += barH + gap;
        ++ci;
    }
}

void PaperSecretScanner::drawStats(QPainter& p, const QRect& rect)
{
    p.fillRect(rect, QColor("#1e293b"));

    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.setPen(Qt::white);
    p.drawText(rect.adjusted(8, 6, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont = p.font();
    statFont.setBold(false);
    statFont.setPointSize(10);
    p.setFont(statFont);

    const int total    = entries_.size();
    const int critical = criticalCount();
    const qreal avgC   = avgConfidence();

    int y = rect.y() + 36;
    const int x = rect.x() + 12;
    const int lineH = 24;

    // Total findings
    p.setPen(QColor("#3b82f6"));
    p.drawText(QRect(x, y, rect.width() - 24, lineH), Qt::AlignLeft | Qt::AlignVCenter,
               QString("Total Findings: %1").arg(total));
    y += lineH;

    // Critical count
    p.setPen(QColor("#dc2626"));
    p.drawText(QRect(x, y, rect.width() - 24, lineH), Qt::AlignLeft | Qt::AlignVCenter,
               QString("Critical: %1").arg(critical));
    y += lineH;

    // Avg confidence
    p.setPen(QColor("#16a34a"));
    p.drawText(QRect(x, y, rect.width() - 24, lineH), Qt::AlignLeft | Qt::AlignVCenter,
               QString("Avg Confidence: %1%").arg(avgC * 100, 0, 'f', 1));
    y += lineH + 8;

    // Severity breakdown
    static const QStringList sevs = {"high", "medium", "low"};
    static const QVector<QColor> sevColors = {
        QColor("#d97706"), QColor("#3b82f6"), QColor("#16a34a")
    };
    for (int i = 0; i < sevs.size(); ++i) {
        int cnt = 0;
        for (const auto& e : entries_)
            if (e.severity == sevs[i]) ++cnt;
        p.setPen(sevColors[i]);
        p.drawText(QRect(x, y, rect.width() - 24, lineH), Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1: %2").arg(sevs[i].left(1).toUpper() + sevs[i].mid(1)).arg(cnt));
        y += lineH;
    }
}

// ── Info label ──────────────────────────────────────────────────────────────

void PaperSecretScanner::updateInfo()
{
    const int total    = entries_.size();
    const int critical = criticalCount();
    const qreal avg    = avgConfidence();
    infoLabel_->setText(
        QString("Findings: %1 | Critical: %2 | Avg Confidence: %3%")
            .arg(total)
            .arg(critical)
            .arg(avg * 100, 0, 'f', 1));
}
