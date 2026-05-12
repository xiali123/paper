#include "tools/PaperWebhookTester.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QDateTime>
#include <QPainterPath>
#include <QFontMetrics>

PaperWebhookTester::PaperWebhookTester(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "WebhookTester")
{
    setupUI();
    loadSettings();
}

void PaperWebhookTester::setupUI()
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItem("All");
    categoryCombo_->addItem("GitHub");
    categoryCombo_->addItem("Slack");
    categoryCombo_->addItem("Discord");
    categoryCombo_->addItem("Custom");
    categoryCombo_->setMinimumWidth(90);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Webhook URL...");

    testBtn_ = new QPushButton("Test", this);
    clearBtn_ = new QPushButton("Clear", this);

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addWidget(categoryCombo_);
    layout->addWidget(inputField_, 1);
    layout->addWidget(testBtn_);
    layout->addWidget(clearBtn_);
    layout->addWidget(infoLabel_);
    layout->addStretch();

    connect(testBtn_, &QPushButton::clicked, this, &PaperWebhookTester::onTest);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWebhookTester::onClear);

    updateInfo();
}

void PaperWebhookTester::loadSettings()
{
    settings_.beginGroup("WebhookTester");
    int count = settings_.beginReadArray("entries");
    entries_.clear();
    for (int i = 0; i < count; ++i) {
        settings_.setArrayIndex(i);
        WebhookEntry entry;
        entry.id = settings_.value("id", i + 1).toInt();
        entry.url = settings_.value("url").toString();
        entry.category = settings_.value("category").toString();
        entry.method = settings_.value("method").toString();
        entry.responseTime = settings_.value("responseTime").toReal();
        entry.statusCode = settings_.value("statusCode").toInt();
        entry.success = settings_.value("success").toBool();
        entry.color = QColor(settings_.value("color", "#3b82f6").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
    update();
}

void PaperWebhookTester::saveSettings()
{
    settings_.beginGroup("WebhookTester");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        const auto& entry = entries_[i];
        settings_.setValue("id", entry.id);
        settings_.setValue("url", entry.url);
        settings_.setValue("category", entry.category);
        settings_.setValue("method", entry.method);
        settings_.setValue("responseTime", entry.responseTime);
        settings_.setValue("statusCode", entry.statusCode);
        settings_.setValue("success", entry.success);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperWebhookTester::onTest()
{
    QString url = inputField_->text().trimmed();
    if (url.isEmpty()) {
        return;
    }

    static const QStringList methods = {"GET", "POST", "PUT", "DELETE"};
    QString method = methods[QRandomGenerator::global()->bounded(methods.size())];

    static const QList<int> statusCodes = {200, 201, 400, 401, 403, 404, 500, 502};
    int statusCode = statusCodes[QRandomGenerator::global()->bounded(statusCodes.size())];

    qreal responseTime = 50.0 + QRandomGenerator::global()->bounded(1951);
    bool success = (statusCode >= 200 && statusCode <= 299);

    QString category = categoryCombo_->currentText();
    if (category == "All") {
        category = "Custom";
    }

    QColor color;
    if (statusCode >= 200 && statusCode <= 299) {
        color = QColor("#16a34a");
    } else if (statusCode >= 400 && statusCode <= 499) {
        color = QColor("#d97706");
    } else {
        color = QColor("#dc2626");
    }

    WebhookEntry entry;
    entry.id = static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 100000);
    entry.url = url;
    entry.category = category;
    entry.method = method;
    entry.responseTime = responseTime;
    entry.statusCode = statusCode;
    entry.success = success;
    entry.color = color;

    addEntry(entry);
    saveSettings();
    updateInfo();
    update();
}

void PaperWebhookTester::onClear()
{
    entries_.clear();
    inputField_->clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperWebhookTester::addEntry(const WebhookEntry& entry)
{
    entries_.append(entry);
    emit webhookTested(entry.id, entry.responseTime);
}

QList<WebhookEntry> PaperWebhookTester::entries() const
{
    return entries_;
}

int PaperWebhookTester::successCount() const
{
    int count = 0;
    for (const auto& entry : entries_) {
        if (entry.success) {
            ++count;
        }
    }
    return count;
}

qreal PaperWebhookTester::avgResponseTime() const
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

QMap<QString, int> PaperWebhookTester::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& entry : entries_) {
        counts[entry.category]++;
    }
    return counts;
}

void PaperWebhookTester::updateInfo()
{
    int total = entries_.size();
    int success = successCount();
    qreal avg = avgResponseTime();
    infoLabel_->setText(QString("Webhooks: %1 | Success: %2 | Avg: %3ms")
                            .arg(total)
                            .arg(success)
                            .arg(avg, 0, 'f', 1));
}

void PaperWebhookTester::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int toolbarH = 50;
    int contentH = height() - toolbarH;
    if (contentH < 50) {
        return;
    }

    int w = width();
    int col1W = w * 4 / 10;
    int col2W = w * 3 / 10;
    int col3W = w - col1W - col2W;

    QRect listRect(0, toolbarH, col1W, contentH);
    QRect chartRect(col1W, toolbarH, col2W, contentH);
    QRect statsRect(col1W + col2W, toolbarH, col3W, contentH);

    // Background
    p.fillRect(rect(), QColor("#1e1e2e"));

    // Column backgrounds with subtle borders
    QColor bgCol("#252536");
    QColor borderCol("#3b3b50");

    p.fillRect(listRect, bgCol);
    p.fillRect(chartRect, bgCol);
    p.fillRect(statsRect, bgCol);

    p.setPen(borderCol);
    p.drawLine(col1W, toolbarH, col1W, height());
    p.drawLine(col1W + col2W, toolbarH, col1W + col2W, height());

    drawWebhookList(p, listRect);
    drawCategoryChart(p, chartRect);
    drawStats(p, statsRect);
}

void PaperWebhookTester::drawWebhookList(QPainter& p, const QRect& rect)
{
    QColor titleColor("#e0e0e0");
    QColor textColor("#c0c0c0");
    QColor dimColor("#808090");

    // Title
    p.setPen(titleColor);
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, -12, 0), Qt::AlignLeft | Qt::AlignTop, "Webhook Tester");

    // List entries
    QFont normalFont;
    normalFont.setPointSize(9);
    normalFont.setBold(false);
    p.setFont(normalFont);

    int y = rect.top() + 38;
    int maxBarWidth = rect.width() - 160;
    qreal maxResponseTime = 2000.0;

    int visibleCount = qMin(entries_.size(), (rect.height() - 50) / 36);
    int startIdx = qMax(0, entries_.size() - visibleCount);

    for (int i = startIdx; i < entries_.size() && y + 32 < rect.bottom(); ++i) {
        const auto& entry = entries_[i];

        // Response time bar
        int barWidth = static_cast<int>((entry.responseTime / maxResponseTime) * maxBarWidth);
        barWidth = qBound(4, barWidth, maxBarWidth);

        p.setPen(Qt::NoPen);
        p.setBrush(entry.color);
        p.drawRoundedRect(rect.left() + 12, y, barWidth, 6, 3, 3);

        // Status badge
        p.setPen(entry.color);
        p.drawText(rect.left() + 16 + barWidth, y + 8, QString("[%1]").arg(entry.statusCode));

        // Method + URL (truncated)
        p.setPen(textColor);
        QString methodTag = entry.method.left(4);
        QString displayUrl = entry.url;
        QFontMetrics fm(normalFont);
        int urlMaxWidth = rect.width() - 24;
        displayUrl = fm.elidedText(methodTag + " " + displayUrl, Qt::ElideRight, urlMaxWidth);
        p.drawText(rect.left() + 12, y + 24, displayUrl);

        // Response time on the right
        p.setPen(dimColor);
        QString timeStr = QString("%1ms").arg(entry.responseTime, 0, 'f', 0);
        p.drawText(rect.right() - 60, y + 8, timeStr);

        y += 36;
    }

    if (entries_.isEmpty()) {
        p.setPen(dimColor);
        p.drawText(rect.adjusted(12, 0, -12, 0), Qt::AlignCenter, "No webhooks tested yet");
    }
}

void PaperWebhookTester::drawCategoryChart(QPainter& p, const QRect& rect)
{
    QColor titleColor("#e0e0e0");
    QColor dimColor("#808090");

    // Title
    p.setPen(titleColor);
    QFont titleFont = p.font();
    titleFont.setBold(true);
    titleFont.setPointSize(11);
    p.setFont(titleFont);
    p.drawText(rect.adjusted(12, 10, -12, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

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

    static const QList<QColor> barColors = {
        QColor("#3b82f6"), QColor("#7c3aed"), QColor("#16a34a"),
        QColor("#d97706"), QColor("#dc2626")
    };

    int y = rect.top() + 40;
    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        maxCount = qMax(maxCount, it.value());
    }
    if (maxCount == 0) maxCount = 1;

    int maxBarWidth = rect.width() - 100;
    int colorIdx = 0;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + 28 > rect.bottom()) break;

        QString category = it.key();
        int count = it.value();
        int barWidth = static_cast<int>((static_cast<qreal>(count) / maxCount) * maxBarWidth);
        barWidth = qBound(4, barWidth, maxBarWidth);

        QColor barColor = barColors[colorIdx % barColors.size()];

        // Category label
        p.setPen(QColor("#c0c0c0"));
        p.drawText(rect.left() + 12, y + 12, category);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(rect.left() + 80, y + 2, barWidth, 14, 4, 4);

        // Count
        p.setPen(QColor("#e0e0e0"));
        p.drawText(rect.left() + 86 + barWidth, y + 14, QString::number(count));

        y += 28;
        ++colorIdx;
    }
}

void PaperWebhookTester::drawStats(QPainter& p, const QRect& rect)
{
    QColor titleColor("#e0e0e0");
    QColor dimColor("#808090");
    QColor valueColor("#e0e0e0");

    // Title
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
    valueFont.setPointSize(18);
    valueFont.setBold(true);

    int total = entries_.size();
    int success = successCount();
    qreal avg = avgResponseTime();

    // Stat blocks
    int blockH = 70;
    int y = rect.top() + 42;
    int leftPad = rect.left() + 16;
    int valueW = rect.width() - 32;

    // Total webhooks
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Total Webhooks");
    p.setPen(QColor("#3b82f6"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 28, QString::number(total));

    y += blockH;

    // Success count
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Success Count");
    p.setPen(QColor("#16a34a"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 28, QString::number(success));

    y += blockH;

    // Avg response time
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Avg Response Time");
    p.setPen(QColor("#7c3aed"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 28, QString("%1ms").arg(avg, 0, 'f', 1));

    y += blockH;

    // Success rate
    qreal rate = total > 0 ? (static_cast<qreal>(success) / total) * 100.0 : 0.0;
    p.setPen(dimColor);
    p.setFont(normalFont);
    p.drawText(leftPad, y, "Success Rate");
    p.setPen(QColor("#d97706"));
    p.setFont(valueFont);
    p.drawText(leftPad, y + 28, QString("%1%").arg(rate, 0, 'f', 1));

    // Decorative mini bar at bottom
    if (total > 0) {
        y += blockH + 10;
        int barW = valueW;
        int successW = static_cast<int>((static_cast<qreal>(success) / total) * barW);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#dc2626"));
        p.drawRoundedRect(leftPad, y, barW, 8, 4, 4);
        p.setBrush(QColor("#16a34a"));
        p.drawRoundedRect(leftPad, y, successW, 8, 4, 4);
    }
}
