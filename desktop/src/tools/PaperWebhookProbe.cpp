#include "tools/PaperWebhookProbe.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPainterPath>
#include <QFontMetrics>

PaperWebhookProbe::PaperWebhookProbe(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperWebhookProbe")
{
    setupUI();
    loadSettings();
}

void PaperWebhookProbe::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "POST", "GET", "PUT", "DELETE"});
    categoryCombo_->setMinimumWidth(90);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter URL...");

    testBtn_ = new QPushButton("Test", this);
    clearBtn_ = new QPushButton("Clear", this);

    toolbar->addWidget(categoryCombo_);
    toolbar->addWidget(inputField_, 1);
    toolbar->addWidget(testBtn_);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();

    connect(testBtn_, &QPushButton::clicked, this, &PaperWebhookProbe::onTest);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWebhookProbe::onClear);

    updateInfo();
}

void PaperWebhookProbe::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int toolbarH = 80;
    int contentH = height() - toolbarH;
    if (contentH < 50) {
        return;
    }

    int w = width();
    int topH = contentH * 3 / 5;
    int botH = contentH - topH;
    int leftW = w / 2;
    int rightW = w - leftW;

    QRect topRect(0, toolbarH, w, topH);
    QRect blRect(0, toolbarH + topH, leftW, botH);
    QRect brRect(leftW, toolbarH + topH, rightW, botH);

    QColor bg("#1e1e2e");
    QColor panelBg("#252536");
    QColor borderCol("#3b3b50");

    p.fillRect(rect(), bg);

    p.fillRect(topRect, panelBg);
    p.fillRect(blRect, panelBg);
    p.fillRect(brRect, panelBg);

    p.setPen(borderCol);
    p.drawLine(0, toolbarH + topH, w, toolbarH + topH);
    p.drawLine(leftW, toolbarH + topH, leftW, height());

    drawProbeView(p, topRect);
    drawCategoryChart(p, blRect);
    drawStats(p, brRect);
}

void PaperWebhookProbe::drawProbeView(QPainter& p, const QRect& rect)
{
    QColor titleColor("#e0e0e0");
    QColor textColor("#c0c0c0");
    QColor dimColor("#808090");

    p.setPen(titleColor);
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, -12, 0), Qt::AlignLeft | Qt::AlignTop, "Webhook Probe");

    QFont normalFont;
    normalFont.setPointSize(9);
    normalFont.setBold(false);
    p.setFont(normalFont);

    QFont smallFont;
    smallFont.setPointSize(8);
    smallFont.setBold(true);

    QFontMetrics fm(normalFont);

    static const QMap<QString, QColor> methodColors = {
        {"POST",   QColor("#3b82f6")},
        {"GET",    QColor("#16a34a")},
        {"PUT",    QColor("#7c3aed")},
        {"DELETE", QColor("#d97706")}
    };

    int y = rect.top() + 40;
    int rowH = 42;
    qreal maxResponseTime = 2000.0;

    int visibleCount = qMin(entries_.size(), (rect.height() - 50) / rowH);
    int startIdx = qMax(0, entries_.size() - visibleCount);

    for (int i = startIdx; i < entries_.size() && y + rowH <= rect.bottom(); ++i) {
        const auto& entry = entries_[i];

        QColor methodColor = methodColors.value(entry.method, QColor("#3b82f6"));

        // Method badge
        p.setPen(Qt::NoPen);
        p.setBrush(methodColor);
        int badgeW = 50;
        int badgeH = 18;
        int badgeX = rect.left() + 12;
        int badgeY = y + 2;
        QPainterPath badgePath;
        badgePath.addRoundedRect(badgeX, badgeY, badgeW, badgeH, 4, 4);
        p.drawPath(badgePath);

        p.setPen(QColor("#ffffff"));
        p.setFont(smallFont);
        p.drawText(QRect(badgeX, badgeY, badgeW, badgeH), Qt::AlignCenter, entry.method);
        p.setFont(normalFont);

        // URL (truncated)
        p.setPen(textColor);
        int urlStartX = badgeX + badgeW + 8;
        int urlMaxW = rect.width() - 220;
        QString displayUrl = fm.elidedText(entry.url, Qt::ElideRight, urlMaxW);
        p.drawText(urlStartX, y + 14, displayUrl);

        // Response time bar
        int barX = rect.right() - 140;
        int barMaxW = 80;
        int barW = static_cast<int>((entry.responseTime / maxResponseTime) * barMaxW);
        barW = qBound(4, barW, barMaxW);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#3b3b50"));
        p.drawRoundedRect(barX, y + 5, barMaxW, 8, 4, 4);
        p.setBrush(methodColor);
        p.drawRoundedRect(barX, y + 5, barW, 8, 4, 4);

        // Response time text
        p.setPen(dimColor);
        p.drawText(barX + barMaxW + 4, y + 13,
                   QString("%1ms").arg(entry.responseTime, 0, 'f', 0));

        // Deliveries count
        p.setPen(QColor("#94a3b8"));
        p.drawText(rect.left() + 12, y + 34,
                   QString("Deliveries: %1").arg(entry.deliveries));

        // Success/fail indicator
        if (entry.success) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#16a34a"));
            QPainterPath checkPath;
            checkPath.addEllipse(rect.right() - 24, y + 26, 12, 12);
            p.drawPath(checkPath);
            p.setPen(QColor("#ffffff"));
            p.setFont(smallFont);
            p.drawText(QRect(rect.right() - 26, y + 25, 16, 14), Qt::AlignCenter, QString::fromUtf8("\xe2\x9c\x93"));
            p.setFont(normalFont);
        } else {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#dc2626"));
            QPainterPath failPath;
            failPath.addEllipse(rect.right() - 24, y + 26, 12, 12);
            p.drawPath(failPath);
            p.setPen(QColor("#ffffff"));
            p.setFont(smallFont);
            p.drawText(QRect(rect.right() - 26, y + 25, 16, 14), Qt::AlignCenter, QString::fromUtf8("\xc3\x97"));
            p.setFont(normalFont);
        }

        y += rowH;
    }

    if (entries_.isEmpty()) {
        p.setPen(dimColor);
        p.drawText(rect.adjusted(12, 0, -12, 0), Qt::AlignCenter, "No webhooks probed yet");
    }
}

void PaperWebhookProbe::drawCategoryChart(QPainter& p, const QRect& rect)
{
    QColor titleColor("#e0e0e0");
    QColor dimColor("#808090");

    p.setPen(titleColor);
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, -12, 0), Qt::AlignLeft | Qt::AlignTop, "Category Chart");

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        QFont normalFont;
        normalFont.setPointSize(9);
        normalFont.setBold(false);
        p.setFont(normalFont);
        p.setPen(dimColor);
        p.drawText(rect.adjusted(12, 0, -12, 0), Qt::AlignCenter, "No data");
        return;
    }

    QFont normalFont;
    normalFont.setPointSize(9);
    normalFont.setBold(false);
    p.setFont(normalFont);

    static const QMap<QString, QColor> catColors = {
        {"POST",   QColor("#3b82f6")},
        {"GET",    QColor("#16a34a")},
        {"PUT",    QColor("#7c3aed")},
        {"DELETE", QColor("#d97706")}
    };

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        maxCount = qMax(maxCount, it.value());
    }
    if (maxCount == 0) maxCount = 1;

    int y = rect.top() + 40;
    int maxBarWidth = rect.width() - 100;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + 28 > rect.bottom()) break;

        QString category = it.key();
        int count = it.value();
        int barW = static_cast<int>((static_cast<qreal>(count) / maxCount) * maxBarWidth);
        barW = qBound(4, barW, maxBarWidth);

        QColor barColor = catColors.value(category, QColor("#3b82f6"));

        p.setPen(QColor("#c0c0c0"));
        p.drawText(rect.left() + 12, y + 14, category);

        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        QPainterPath barPath;
        barPath.addRoundedRect(rect.left() + 80, y + 3, barW, 16, 4, 4);
        p.drawPath(barPath);

        p.setPen(QColor("#e0e0e0"));
        p.drawText(rect.left() + 86 + barW, y + 16, QString::number(count));

        y += 28;
    }
}

void PaperWebhookProbe::drawStats(QPainter& p, const QRect& rect)
{
    QColor titleColor("#e0e0e0");
    QColor dimColor("#808090");

    p.setPen(titleColor);
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, -12, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont normalFont;
    normalFont.setPointSize(9);
    normalFont.setBold(false);
    p.setFont(normalFont);

    QFont valueFont;
    valueFont.setPointSize(16);
    valueFont.setBold(true);

    int total = entries_.size();
    int success = successCount();
    qreal avg = avgResponseTime();

    int y = rect.top() + 42;
    int leftPad = rect.left() + 16;
    int blockH = 56;

    // Total entries
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Total Webhooks");
    p.setPen(QColor("#3b82f6"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 24, QString::number(total));

    y += blockH;

    // Average response time
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Avg Response Time");
    p.setPen(QColor("#7c3aed"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 24, QString("%1ms").arg(avg, 0, 'f', 1));

    y += blockH;

    // Success count
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Success");
    p.setPen(QColor("#16a34a"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 24, QString::number(success));

    y += blockH;

    // Success rate with mini bar
    qreal rate = total > 0 ? (static_cast<qreal>(success) / total) * 100.0 : 0.0;
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Success Rate");
    p.setPen(QColor("#d97706"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 24, QString("%1%").arg(rate, 0, 'f', 1));

    if (total > 0) {
        y += blockH + 4;
        int barW = rect.width() - 32;
        int successW = static_cast<int>((static_cast<qreal>(success) / total) * barW);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#dc2626"));
        QPainterPath bgBar;
        bgBar.addRoundedRect(leftPad, y, barW, 8, 4, 4);
        p.drawPath(bgBar);
        p.setBrush(QColor("#16a34a"));
        QPainterPath fgBar;
        fgBar.addRoundedRect(leftPad, y, successW, 8, 4, 4);
        p.drawPath(fgBar);
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.setFont(normalFont);
        p.drawText(rect.adjusted(12, 0, -12, 0), Qt::AlignCenter, "No data");
    }
}

void PaperWebhookProbe::onTest()
{
    QString url = inputField_->text().trimmed();
    if (url.isEmpty()) {
        return;
    }

    static const QStringList methods = {"POST", "GET", "PUT", "DELETE"};
    static const QMap<QString, QColor> methodColors = {
        {"POST",   QColor("#3b82f6")},
        {"GET",    QColor("#16a34a")},
        {"PUT",    QColor("#7c3aed")},
        {"DELETE", QColor("#d97706")}
    };

    QString selectedFilter = categoryCombo_->currentText();
    QString method;
    if (selectedFilter == "All") {
        method = methods[QRandomGenerator::global()->bounded(methods.size())];
    } else {
        method = selectedFilter;
    }

    qreal responseTime = 30.0 + QRandomGenerator::global()->bounded(1971);
    int deliveries = QRandomGenerator::global()->bounded(1, 101);
    bool success = QRandomGenerator::global()->bounded(100) < 75;

    WebhookProbeEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.url = url;
    entry.category = method;
    entry.method = method;
    entry.responseTime = responseTime;
    entry.deliveries = deliveries;
    entry.success = success;
    entry.color = methodColors.value(method, QColor("#3b82f6"));

    addEntry(entry);
    saveSettings();
    updateInfo();
    update();
}

void PaperWebhookProbe::onClear()
{
    entries_.clear();
    inputField_->clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperWebhookProbe::updateInfo()
{
    int total = entries_.size();
    int success = successCount();
    qreal avg = avgResponseTime();
    infoLabel_->setText(QString("Webhooks: %1 | Success: %2 | Avg: %3ms")
                            .arg(total)
                            .arg(success)
                            .arg(avg, 0, 'f', 1));
}

void PaperWebhookProbe::loadSettings()
{
    settings_.beginGroup("WebhookProbe");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        WebhookProbeEntry entry;
        entry.id = settings_.value("id", i + 1).toInt();
        entry.url = settings_.value("url").toString();
        entry.category = settings_.value("category").toString();
        entry.method = settings_.value("method").toString();
        entry.responseTime = settings_.value("responseTime").toReal();
        entry.deliveries = settings_.value("deliveries").toInt();
        entry.success = settings_.value("success").toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperWebhookProbe::saveSettings()
{
    settings_.beginGroup("WebhookProbe");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("url", entry.url);
        settings_.setValue("category", entry.category);
        settings_.setValue("method", entry.method);
        settings_.setValue("responseTime", entry.responseTime);
        settings_.setValue("deliveries", entry.deliveries);
        settings_.setValue("success", entry.success);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperWebhookProbe::addEntry(const WebhookProbeEntry& entry)
{
    entries_.append(entry);
    emit webhookTested(entry.id, entry.responseTime);
}

QList<WebhookProbeEntry> PaperWebhookProbe::entries() const
{
    return entries_;
}

int PaperWebhookProbe::successCount() const
{
    int count = 0;
    for (const auto& entry : entries_) {
        if (entry.success) {
            ++count;
        }
    }
    return count;
}

qreal PaperWebhookProbe::avgResponseTime() const
{
    if (entries_.isEmpty()) {
        return 0.0;
    }
    qreal total = 0.0;
    for (const auto& entry : entries_) {
        total += entry.responseTime;
    }
    return total / entries_.size();
}

QMap<QString, int> PaperWebhookProbe::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& entry : entries_) {
        counts[entry.category]++;
    }
    return counts;
}
