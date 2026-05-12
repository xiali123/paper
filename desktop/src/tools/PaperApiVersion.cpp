#include "tools/PaperApiVersion.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPaintEvent>
#include <QPainterPath>
#include <algorithm>
#include <cmath>

PaperApiVersion::PaperApiVersion(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "ApiVersion")
{
    setupUI();
    loadSettings();
    updateInfo();
}

void PaperApiVersion::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Top control bar
    auto* topBar = new QHBoxLayout();
    topBar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem("All");
    categoryCombo_->addItem("REST");
    categoryCombo_->addItem("GraphQL");
    categoryCombo_->addItem("WebSocket");
    categoryCombo_->addItem("gRPC");
    topBar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter endpoint...");
    topBar->addWidget(inputField_);

    checkBtn_ = new QPushButton("Check", this);
    connect(checkBtn_, &QPushButton::clicked, this, &PaperApiVersion::onCheck);
    topBar->addWidget(checkBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperApiVersion::onClear);
    topBar->addWidget(clearBtn_);

    mainLayout->addLayout(topBar);

    // Bottom info label
    infoLabel_ = new QLabel(this);
    infoLabel_->setWordWrap(true);
    mainLayout->addWidget(infoLabel_);

    // Canvas area for paintEvent drawing
    mainLayout->addStretch(1);

    setMinimumSize(800, 500);
}

void PaperApiVersion::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Background
    p.fillRect(rect(), QColor("#1e1e2e"));

    // Layout: top half = version view, bottom split into chart (left) + stats (right)
    int topBarHeight = 80; // approximate top controls + info label height
    int margin = 12;
    int spacing = 8;

    int canvasTop = topBarHeight;
    int canvasW = w - 2 * margin;
    int canvasH = h - canvasTop - margin;

    // Top half: version view
    int topHalfH = canvasH / 2 - spacing / 2;
    QRect versionRect(margin, canvasTop, canvasW, topHalfH);
    drawVersionView(p, versionRect);

    // Bottom half split into chart (left) and stats (right)
    int bottomY = canvasTop + topHalfH + spacing;
    int bottomH = canvasH - topHalfH - spacing;
    int halfW = (canvasW - spacing) / 2;

    QRect chartRect(margin, bottomY, halfW, bottomH);
    drawCategoryChart(p, chartRect);

    QRect statsRect(margin + halfW + spacing, bottomY, halfW, bottomH);
    drawStats(p, statsRect);
}

void PaperApiVersion::drawVersionView(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#2a2a3c"));
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.drawPath(bgPath);

    // Title
    p.setPen(QColor("#e0e0e0"));
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "API Version Monitor");

    if (entries_.isEmpty()) {
        QFont emptyFont = p.font();
        emptyFont.setBold(false);
        emptyFont.setPointSize(9);
        p.setFont(emptyFont);
        p.setPen(QColor("#666"));
        p.drawText(rect, Qt::AlignCenter, "No endpoints checked");
        return;
    }

    // Category color map
    static const QMap<QString, QColor> catColors = {
        {"REST",     QColor("#3b82f6")},
        {"GraphQL",  QColor("#16a34a")},
        {"WebSocket",QColor("#7c3aed")},
        {"gRPC",     QColor("#d97706")}
    };

    QFont entryFont = p.font();
    entryFont.setBold(false);
    entryFont.setPointSize(8);
    p.setFont(entryFont);

    int y = rect.top() + 36;
    int itemHeight = 32;
    int contentLeft = rect.left() + 12;
    int contentWidth = rect.width() - 24;
    int visibleCount = qMin(entries_.size(), (rect.height() - 50) / itemHeight);

    for (int i = 0; i < visibleCount; ++i) {
        const auto& entry = entries_[i];
        int itemY = y + i * itemHeight;
        QColor catColor = catColors.value(entry.category, QColor("#888888"));

        // Row background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#32324a"));
        QPainterPath rowPath;
        rowPath.addRoundedRect(QRect(contentLeft, itemY, contentWidth, itemHeight - 2), 4, 4);
        p.drawPath(rowPath);

        // Category indicator dot
        p.setBrush(catColor);
        p.drawEllipse(contentLeft + 6, itemY + 10, 8, 8);

        // Endpoint text
        p.setPen(entry.deprecated ? QColor("#dc2626") : QColor("#d0d0d0"));
        QString endpoint = entry.endpoint;
        if (endpoint.length() > 28)
            endpoint = endpoint.left(25) + "...";
        p.drawText(contentLeft + 20, itemY, contentWidth - 20, itemHeight - 2,
                   Qt::AlignLeft | Qt::AlignVCenter, endpoint);

        // Version badge
        int badgeX = contentLeft + contentWidth - 240;
        int badgeW = 58;
        int badgeH = 18;
        int badgeY = itemY + (itemHeight - 2 - badgeH) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(catColor);
        QPainterPath badgePath;
        badgePath.addRoundedRect(QRect(badgeX, badgeY, badgeW, badgeH), 9, 9);
        p.drawPath(badgePath);
        p.setPen(Qt::white);
        QFont badgeFont = entryFont;
        badgeFont.setBold(true);
        badgeFont.setPointSize(7);
        p.setFont(badgeFont);
        p.drawText(QRect(badgeX, badgeY, badgeW, badgeH), Qt::AlignCenter, "v" + entry.version);
        p.setFont(entryFont);

        // Latency bar
        int barX = badgeX + badgeW + 10;
        int barW = 100;
        int barH = 6;
        int barY = itemY + (itemHeight - 2 - barH) / 2;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#3a3a4c"));
        QPainterPath barBg;
        barBg.addRoundedRect(QRect(barX, barY, barW, barH), 3, 3);
        p.drawPath(barBg);

        int latencyFill = qMin(static_cast<int>(entry.latency * barW / 500.0), barW);
        QColor latencyColor = entry.latency < 100 ? QColor("#16a34a") :
                              entry.latency < 300 ? QColor("#d97706") : QColor("#dc2626");
        if (latencyFill > 0) {
            p.setBrush(latencyColor);
            QPainterPath barFill;
            barFill.addRoundedRect(QRect(barX, barY, latencyFill, barH), 3, 3);
            p.drawPath(barFill);
        }

        // Call count
        p.setPen(QColor("#aaa"));
        p.drawText(barX + barW + 8, itemY, 40, itemHeight - 2,
                   Qt::AlignLeft | Qt::AlignVCenter, QString("%1x").arg(entry.calls));

        // Deprecated warning
        if (entry.deprecated) {
            p.setPen(QColor("#dc2626"));
            QFont warnFont = entryFont;
            warnFont.setBold(true);
            p.setFont(warnFont);
            p.drawText(contentLeft + contentWidth - 60, itemY, 55, itemHeight - 2,
                       Qt::AlignRight | Qt::AlignVCenter, "DEPR");
            p.setFont(entryFont);
        }
    }
}

void PaperApiVersion::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#2a2a3c"));
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.drawPath(bgPath);

    // Title
    p.setPen(QColor("#e0e0e0"));
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Category Distribution");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont emptyFont = p.font();
        emptyFont.setBold(false);
        emptyFont.setPointSize(9);
        p.setFont(emptyFont);
        p.setPen(QColor("#666"));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    static const QMap<QString, QColor> catColors = {
        {"REST",      QColor("#3b82f6")},
        {"GraphQL",   QColor("#16a34a")},
        {"WebSocket", QColor("#7c3aed")},
        {"gRPC",      QColor("#d97706")}
    };

    QFont labelFont = p.font();
    labelFont.setBold(false);
    labelFont.setPointSize(8);
    p.setFont(labelFont);

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it)
        maxCount = qMax(maxCount, it.value());

    int y = rect.top() + 40;
    int barAreaWidth = rect.width() - 110;
    int barHeight = 22;
    int spacing = 8;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + barHeight > rect.bottom() - 8)
            break;

        QColor barColor = catColors.value(it.key(), QColor("#888888"));

        // Category label
        p.setPen(QColor("#cccccc"));
        p.drawText(rect.left() + 12, y, 72, barHeight, Qt::AlignLeft | Qt::AlignVCenter, it.key());

        // Bar background
        int barX = rect.left() + 90;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#3a3a4c"));
        QPainterPath barBg;
        barBg.addRoundedRect(QRect(barX, y + 3, barAreaWidth, barHeight - 6), 3, 3);
        p.drawPath(barBg);

        // Bar fill
        int fillWidth = maxCount > 0
            ? static_cast<int>(barAreaWidth * static_cast<qreal>(it.value()) / maxCount)
            : 0;
        if (fillWidth > 0) {
            p.setBrush(barColor);
            QPainterPath barFill;
            barFill.addRoundedRect(QRect(barX, y + 3, fillWidth, barHeight - 6), 3, 3);
            p.drawPath(barFill);
        }

        // Count label
        p.setPen(QColor("#aaa"));
        p.drawText(barX + barAreaWidth + 6, y, 30, barHeight,
                   Qt::AlignLeft | Qt::AlignVCenter, QString::number(it.value()));

        y += barHeight + spacing;
    }
}

void PaperApiVersion::drawStats(QPainter& p, const QRect& rect)
{
    // Panel background
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#2a2a3c"));
    QPainterPath bgPath;
    bgPath.addRoundedRect(rect, 8, 8);
    p.drawPath(bgPath);

    // Title
    p.setPen(QColor("#e0e0e0"));
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, 0, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont statFont = p.font();
    statFont.setBold(false);
    statFont.setPointSize(10);
    p.setFont(statFont);

    QFont valFont = statFont;
    valFont.setBold(true);
    int y = rect.top() + 44;
    int lineH = 30;
    int pad = 16;

    // Total endpoints
    p.setFont(statFont);
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.left() + pad, y, rect.width() - 2 * pad, lineH,
               Qt::AlignLeft | Qt::AlignVCenter, "Total Endpoints");
    p.setFont(valFont);
    p.setPen(QColor("#e0e0e0"));
    p.drawText(rect.left() + pad, y, rect.width() - 2 * pad, lineH,
               Qt::AlignRight | Qt::AlignVCenter, QString::number(entries_.size()));
    y += lineH;

    // Separator
    p.setPen(QColor("#3a3a4c"));
    p.drawLine(rect.left() + pad, y - 4, rect.right() - pad, y - 4);

    // Deprecated count
    p.setFont(statFont);
    p.setPen(QColor("#dc2626"));
    p.drawText(rect.left() + pad, y, rect.width() - 2 * pad, lineH,
               Qt::AlignLeft | Qt::AlignVCenter, "Deprecated");
    p.setFont(valFont);
    p.setPen(QColor("#e0e0e0"));
    p.drawText(rect.left() + pad, y, rect.width() - 2 * pad, lineH,
               Qt::AlignRight | Qt::AlignVCenter, QString::number(deprecatedCount()));
    y += lineH;

    // Separator
    p.setPen(QColor("#3a3a4c"));
    p.drawLine(rect.left() + pad, y - 4, rect.right() - pad, y - 4);

    // Average latency
    p.setFont(statFont);
    p.setPen(QColor("#d97706"));
    p.drawText(rect.left() + pad, y, rect.width() - 2 * pad, lineH,
               Qt::AlignLeft | Qt::AlignVCenter, "Avg Latency");
    p.setFont(valFont);
    p.setPen(QColor("#e0e0e0"));
    qreal avgLat = avgLatency();
    p.drawText(rect.left() + pad, y, rect.width() - 2 * pad, lineH,
               Qt::AlignRight | Qt::AlignVCenter, QString("%1 ms").arg(avgLat, 0, 'f', 1));
    y += lineH;

    // Separator
    p.setPen(QColor("#3a3a4c"));
    p.drawLine(rect.left() + pad, y - 4, rect.right() - pad, y - 4);

    // Health bar
    p.setFont(statFont);
    p.setPen(QColor("#16a34a"));
    p.drawText(rect.left() + pad, y, rect.width() - 2 * pad, lineH,
               Qt::AlignLeft | Qt::AlignVCenter, "API Health");
    y += lineH;

    int barX = rect.left() + pad;
    int barW = rect.width() - 2 * pad;
    int barH = 10;
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#3a3a4c"));
    QPainterPath healthBg;
    healthBg.addRoundedRect(QRect(barX, y, barW, barH), 5, 5);
    p.drawPath(healthBg);

    qreal healthRatio = entries_.isEmpty()
        ? 1.0
        : 1.0 - static_cast<qreal>(deprecatedCount()) / entries_.size();
    QColor healthColor = healthRatio >= 0.7 ? QColor("#16a34a") :
                         healthRatio >= 0.4 ? QColor("#d97706") : QColor("#dc2626");
    int healthWidth = static_cast<int>(barW * healthRatio);
    if (healthWidth > 0) {
        p.setBrush(healthColor);
        QPainterPath healthFill;
        healthFill.addRoundedRect(QRect(barX, y, healthWidth, barH), 5, 5);
        p.drawPath(healthFill);
    }
}

void PaperApiVersion::onCheck()
{
    QString endpoint = inputField_->text().trimmed();
    if (endpoint.isEmpty())
        return;

    static const QStringList categories = {"REST", "GraphQL", "WebSocket", "gRPC"};
    QString category = categoryCombo_->currentText();
    if (category == "All")
        category = categories[QRandomGenerator::global()->bounded(categories.size())];

    // Category-to-color mapping
    static const QMap<QString, QColor> catColors = {
        {"REST",      QColor("#3b82f6")},
        {"GraphQL",   QColor("#16a34a")},
        {"WebSocket", QColor("#7c3aed")},
        {"gRPC",      QColor("#d97706")}
    };

    qreal latency = QRandomGenerator::global()->generateDouble() * 500.0;
    int calls = QRandomGenerator::global()->bounded(1, 1000);
    bool deprecated = latency > 400.0;

    int major = QRandomGenerator::global()->bounded(1, 4);
    int minor = QRandomGenerator::global()->bounded(0, 10);
    int patch = QRandomGenerator::global()->bounded(0, 20);
    QString version = QString("%1.%2.%3").arg(major).arg(minor).arg(patch);

    ApiVersionEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.endpoint = endpoint;
    entry.category = category;
    entry.version = version;
    entry.latency = latency;
    entry.calls = calls;
    entry.deprecated = deprecated;
    entry.color = catColors.value(category, QColor("#888888"));

    entries_.append(entry);
    inputField_->clear();

    saveSettings();
    updateInfo();
    update();
    emit versionChecked(entry.id, entry.latency);
}

void PaperApiVersion::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperApiVersion::addEntry(const ApiVersionEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ApiVersionEntry> PaperApiVersion::entries() const
{
    return entries_;
}

int PaperApiVersion::deprecatedCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.deprecated)
            ++count;
    }
    return count;
}

qreal PaperApiVersion::avgLatency() const
{
    if (entries_.isEmpty())
        return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperApiVersion::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperApiVersion::updateInfo()
{
    int total = entries_.size();
    int deprecated = deprecatedCount();
    qreal avgLat = avgLatency();
    qreal depPercent = total > 0
        ? static_cast<qreal>(deprecated) / total * 100.0
        : 0.0;

    infoLabel_->setText(
        QString("Endpoints: %1 | Deprecated: %2 (%3%) | Avg Latency: %4 ms")
            .arg(total)
            .arg(deprecated)
            .arg(depPercent, 0, 'f', 1)
            .arg(avgLat, 0, 'f', 1));
}

void PaperApiVersion::loadSettings()
{
    settings_.beginGroup("ApiVersion");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        ApiVersionEntry entry;
        entry.id = settings_.value("id", i + 1).toInt();
        entry.endpoint = settings_.value("endpoint").toString();
        entry.category = settings_.value("category", "REST").toString();
        entry.version = settings_.value("version", "1.0.0").toString();
        entry.latency = settings_.value("latency", 0.0).toReal();
        entry.calls = settings_.value("calls", 0).toInt();
        entry.deprecated = settings_.value("deprecated", false).toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
}

void PaperApiVersion::saveSettings()
{
    settings_.beginGroup("ApiVersion");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& e = entries_[i];
        settings_.setValue("id", e.id);
        settings_.setValue("endpoint", e.endpoint);
        settings_.setValue("category", e.category);
        settings_.setValue("version", e.version);
        settings_.setValue("latency", e.latency);
        settings_.setValue("calls", e.calls);
        settings_.setValue("deprecated", e.deprecated);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}
