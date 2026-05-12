#include "tools/PaperEndpointPulse.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFontMetrics>
#include <QtMath>

PaperEndpointPulse::PaperEndpointPulse(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperEndpointPulse")
{
    setupUI();
    loadSettings();
}

void PaperEndpointPulse::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    auto* topBar = new QHBoxLayout();

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "GET", "POST", "PUT", "DELETE"});
    topBar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Enter endpoint...");
    topBar->addWidget(inputField_);

    pulseBtn_ = new QPushButton("Pulse", this);
    connect(pulseBtn_, &QPushButton::clicked, this, &PaperEndpointPulse::onPulse);
    topBar->addWidget(pulseBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEndpointPulse::onClear);
    topBar->addWidget(clearBtn_);

    mainLayout->addLayout(topBar);

    infoLabel_ = new QLabel(this);
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch();
}

void PaperEndpointPulse::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    // Reserve roughly 50px at top for the controls layout
    int topOffset = 52;
    int halfH = topOffset + (h - topOffset) / 2;
    int halfW = w / 2;

    drawPulseView(p, QRect(0, topOffset, w, halfH - topOffset));
    drawCategoryChart(p, QRect(0, halfH, halfW, h - halfH));
    drawStats(p, QRect(halfW, halfH, w - halfW, h - halfH));
}

void PaperEndpointPulse::drawPulseView(QPainter& p, const QRect& rect)
{
    QColor bg("#1e293b");
    QColor border("#334155");
    QColor titleColor("#f8fafc");
    QColor textWhite("#e2e8f0");

    p.fillRect(rect, bg);
    p.setPen(border);
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    p.setFont(titleFont);
    p.setPen(QColor("#3b82f6"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               "Endpoint Pulse");

    QFont normalFont = font();
    p.setFont(normalFont);

    if (entries_.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.drawText(rect, Qt::AlignCenter, "No endpoints pulsed");
        return;
    }

    int y = rect.top() + 38;
    int rowHeight = 52;
    int barMaxWidth = rect.width() - 220;

    QMap<QString, QColor> methodColors;
    methodColors["GET"]    = QColor("#3b82f6");
    methodColors["POST"]   = QColor("#16a34a");
    methodColors["PUT"]    = QColor("#7c3aed");
    methodColors["DELETE"] = QColor("#d97706");

    QFontMetrics fm(p.font());

    for (const auto& entry : entries_) {
        if (y + rowHeight > rect.bottom()) break;

        // --- Heartbeat waveform ---
        QColor waveColor = methodColors.value(entry.method, QColor("#3b82f6"));
        int waveWidth = 80;
        int waveX = rect.left() + 10;
        int waveMidY = y + 14;

        QPainterPath wavePath;
        wavePath.moveTo(waveX, waveMidY);
        qreal step = static_cast<qreal>(waveWidth) / 16.0;
        qreal cx = waveX;
        for (int i = 0; i < 16; ++i) {
            cx += step;
            if (i % 4 == 1) {
                wavePath.lineTo(cx, waveMidY - 10);
            } else if (i % 4 == 2) {
                wavePath.lineTo(cx, waveMidY + 6);
            } else if (i % 4 == 3) {
                wavePath.lineTo(cx, waveMidY - 4);
            } else {
                wavePath.lineTo(cx, waveMidY);
            }
        }

        QPen wavePen(waveColor, 1.5);
        p.setPen(wavePen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(wavePath);

        // --- Endpoint name ---
        p.setPen(textWhite);
        QString epText = fm.elidedText(entry.endpoint, Qt::ElideRight, 200);
        p.drawText(QRect(waveX + waveWidth + 8, y, 200, 20),
                   Qt::AlignLeft | Qt::AlignVCenter, epText);

        // --- Response time bar ---
        int barY = y + 24;
        qreal maxResponse = 2000.0;
        qreal ratio = qMin(entry.responseTime / maxResponse, 1.0);
        int barW = static_cast<int>(barMaxWidth * ratio);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#334155"));
        p.drawRoundedRect(QRect(waveX + waveWidth + 8, barY, barMaxWidth, 8), 3, 3);

        p.setBrush(waveColor);
        if (barW > 0) {
            p.drawRoundedRect(QRect(waveX + waveWidth + 8, barY, barW, 8), 3, 3);
        }

        // --- Response time label ---
        p.setPen(QColor("#94a3b8"));
        p.setFont(normalFont);
        p.drawText(QRect(waveX + waveWidth + 10 + barMaxWidth, barY - 4, 70, 16),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("%1ms").arg(entry.responseTime, 0, 'f', 0));

        // --- Request count ---
        p.setPen(QColor("#94a3b8"));
        p.drawText(QRect(rect.right() - 120, y, 50, 20),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString("x%1").arg(entry.requests));

        // --- Health indicator dot ---
        int dotX = rect.right() - 20;
        int dotY = y + 10;
        p.setPen(Qt::NoPen);
        p.setBrush(entry.healthy ? QColor("#22c55e") : QColor("#ef4444"));
        p.drawEllipse(QPoint(dotX, dotY), 6, 6);

        // Inner highlight on dot
        p.setBrush(entry.healthy ? QColor("#4ade80") : QColor("#f87171"));
        p.drawEllipse(QPoint(dotX, dotY), 3, 3);

        y += rowHeight;
    }
}

void PaperEndpointPulse::drawCategoryChart(QPainter& p, const QRect& rect)
{
    QColor bg("#1e293b");
    QColor border("#334155");

    p.fillRect(rect, bg);
    p.setPen(border);
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    p.setFont(titleFont);
    p.setPen(QColor("#7c3aed"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               "Method Breakdown");

    QFont normalFont = font();
    p.setFont(normalFont);

    QMap<QString, int> counts = categoryCounts();
    if (counts.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    int maxCount = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (it.value() > maxCount) maxCount = it.value();
    }

    QMap<QString, QColor> methodColors;
    methodColors["GET"]    = QColor("#3b82f6");
    methodColors["POST"]   = QColor("#16a34a");
    methodColors["PUT"]    = QColor("#7c3aed");
    methodColors["DELETE"] = QColor("#d97706");

    int y = rect.top() + 40;
    int barMaxWidth = rect.width() - 120;

    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + 30 > rect.bottom()) break;

        p.setPen(QColor("#e2e8f0"));
        p.drawText(QRect(rect.left() + 10, y, 60, 24),
                   Qt::AlignLeft | Qt::AlignVCenter, it.key());

        int barW = maxCount > 0
                       ? static_cast<int>(barMaxWidth * (static_cast<qreal>(it.value()) / maxCount))
                       : 0;

        QColor barColor = methodColors.value(it.key(), QColor("#3b82f6"));
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);

        QPainterPath barPath;
        barPath.addRoundedRect(QRect(rect.left() + 75, y + 4, barW, 16), 3, 3);
        p.drawPath(barPath);

        p.setPen(QColor("#94a3b8"));
        p.drawText(QRect(rect.left() + 80 + barW, y, 30, 24),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(it.value()));

        y += 30;
    }
}

void PaperEndpointPulse::drawStats(QPainter& p, const QRect& rect)
{
    QColor bg("#1e293b");
    QColor border("#334155");

    p.fillRect(rect, bg);
    p.setPen(border);
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    QFont titleFont = font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 2);
    p.setFont(titleFont);
    p.setPen(QColor("#16a34a"));
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               "Pulse Statistics");

    QFont normalFont = font();
    p.setFont(normalFont);

    int y = rect.top() + 45;

    // Total endpoints
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, "Total Endpoints");
    p.setPen(QColor("#f8fafc"));
    p.drawText(QRect(rect.left() + 10, y + 20, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter,
               QString::number(entries_.size()));

    y += 55;

    // Avg response time
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, "Avg Response Time");
    p.setPen(QColor("#3b82f6"));
    p.drawText(QRect(rect.left() + 10, y + 20, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter,
               QString("%1 ms").arg(avgResponseTime(), 0, 'f', 1));

    y += 55;

    // Healthy count
    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, "Healthy Endpoints");
    int healthy = healthyCount();
    p.setPen(healthy > 0 ? QColor("#22c55e") : QColor("#ef4444"));
    p.drawText(QRect(rect.left() + 10, y + 20, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter,
               QString("%1 / %2").arg(healthy).arg(entries_.size()));

    if (entries_.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.drawText(rect, Qt::AlignCenter, "No data");
    }
}

void PaperEndpointPulse::onPulse()
{
    QString endpoint = inputField_->text().trimmed();
    if (endpoint.isEmpty()) return;

    QString category = categoryCombo_->currentText();
    QString method = (category == "All")
                         ? QStringList{"GET", "POST", "PUT", "DELETE"}[QRandomGenerator::global()->bounded(4)]
                         : category;

    qreal responseTime = QRandomGenerator::global()->bounded(50, 2001);
    int requests = QRandomGenerator::global()->bounded(1, 1001);
    bool healthy = responseTime < 1000.0;

    QMap<QString, QColor> methodColors;
    methodColors["GET"]    = QColor("#3b82f6");
    methodColors["POST"]   = QColor("#16a34a");
    methodColors["PUT"]    = QColor("#7c3aed");
    methodColors["DELETE"] = QColor("#d97706");

    EndpointPulseEntry entry;
    entry.id = entries_.size() + 1;
    entry.endpoint = endpoint;
    entry.category = category;
    entry.method = method;
    entry.responseTime = responseTime;
    entry.requests = requests;
    entry.healthy = healthy;
    entry.color = methodColors.value(method, QColor("#3b82f6"));

    addEntry(entry);
    updateInfo();
    saveSettings();
    update();
    emit pulseChecked(entry.id, entry.responseTime);
}

void PaperEndpointPulse::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperEndpointPulse::updateInfo()
{
    int total = entries_.size();
    int healthy = healthyCount();
    qreal avg = avgResponseTime();
    qreal healthyPct = total > 0 ? (static_cast<qreal>(healthy) / total) * 100.0 : 0.0;
    infoLabel_->setText(
        QString("Endpoints: %1 | Healthy: %2% | Avg Response: %3 ms")
            .arg(total)
            .arg(healthyPct, 0, 'f', 1)
            .arg(avg, 0, 'f', 1));
}

void PaperEndpointPulse::loadSettings()
{
    settings_.beginGroup("EndpointPulse");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EndpointPulseEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.endpoint = settings_.value("endpoint").toString();
        entry.category = settings_.value("category").toString();
        entry.method = settings_.value("method").toString();
        entry.responseTime = settings_.value("responseTime").toReal();
        entry.requests = settings_.value("requests").toInt();
        entry.healthy = settings_.value("healthy").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperEndpointPulse::saveSettings()
{
    settings_.beginGroup("EndpointPulse");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id", entry.id);
        settings_.setValue("endpoint", entry.endpoint);
        settings_.setValue("category", entry.category);
        settings_.setValue("method", entry.method);
        settings_.setValue("responseTime", entry.responseTime);
        settings_.setValue("requests", entry.requests);
        settings_.setValue("healthy", entry.healthy);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperEndpointPulse::addEntry(const EndpointPulseEntry& entry)
{
    entries_.append(entry);
}

QList<EndpointPulseEntry> PaperEndpointPulse::entries() const
{
    return entries_;
}

int PaperEndpointPulse::healthyCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.healthy) ++count;
    }
    return count;
}

qreal PaperEndpointPulse::avgResponseTime() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.responseTime;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperEndpointPulse::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.method]++;
    }
    return counts;
}
