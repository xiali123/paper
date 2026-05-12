#include "tools/PaperEndpointMonitor.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QFontMetrics>

PaperEndpointMonitor::PaperEndpointMonitor(QWidget* parent)
    : QWidget(parent)
    , settings_(QSettings::IniFormat, QSettings::UserScope, "PaperCrawler", "PaperEndpointMonitor")
{
    setupUI();
    loadSettings();
}

void PaperEndpointMonitor::setupUI()
{
    auto* layout = new QHBoxLayout(this);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "API", "Database", "Storage", "Auth", "Search"});
    layout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Endpoint URL...");
    layout->addWidget(inputField_);

    checkBtn_ = new QPushButton("Check", this);
    connect(checkBtn_, &QPushButton::clicked, this, &PaperEndpointMonitor::onCheck);
    layout->addWidget(checkBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEndpointMonitor::onClear);
    layout->addWidget(clearBtn_);

    infoLabel_ = new QLabel(this);
    layout->addWidget(infoLabel_);

    layout->addStretch();
}

void PaperEndpointMonitor::onCheck()
{
    QString url = inputField_->text().trimmed();
    if (url.isEmpty()) return;

    QString category = categoryCombo_->currentText();

    qreal uptime = QRandomGenerator::global()->bounded(9000, 10001) / 10000.0;
    int errors = QRandomGenerator::global()->bounded(0, 51);

    bool healthy = uptime > 0.99;
    QString status = healthy ? "up" : (uptime > 0.95 ? "degraded" : "down");

    QColor color;
    if (status == "up") color = QColor("#16a34a");
    else if (status == "degraded") color = QColor("#d97706");
    else color = QColor("#dc2626");

    EndpointMonitorEntry entry;
    entry.id = entries_.size() + 1;
    entry.endpoint = url;
    entry.category = category;
    entry.status = status;
    entry.uptime = uptime;
    entry.errors = errors;
    entry.healthy = healthy;
    entry.color = color;

    addEntry(entry);
    saveSettings();
    updateInfo();
    repaint();
}

void PaperEndpointMonitor::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    repaint();
}

void PaperEndpointMonitor::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int w = width();
    int h = height();

    int col1 = w / 3;
    int col2 = 2 * w / 3;

    drawEndpointList(p, QRect(0, 0, col1, h));
    drawCategoryChart(p, QRect(col1, 0, col2 - col1, h));
    drawStats(p, QRect(col2, 0, w - col2, h));
}

void PaperEndpointMonitor::drawEndpointList(QPainter& p, const QRect& rect)
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
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Endpoint Monitor");

    QFont normalFont = font();
    p.setFont(normalFont);

    int y = rect.top() + 36;
    int barMaxWidth = rect.width() - 20;

    for (const auto& entry : entries_) {
        if (y + 44 > rect.bottom()) break;

        p.setPen(textWhite);
        p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 18),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QFontMetrics(p.font()).elidedText(entry.endpoint, Qt::ElideRight, rect.width() - 60));

        p.setPen(QColor("#94a3b8"));
        p.drawText(QRect(rect.left() + 10, y + 18, 60, 14),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   entry.status);

        int barY = y + 18;
        int barW = static_cast<int>(barMaxWidth * entry.uptime);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#334155"));
        p.drawRoundedRect(QRect(rect.left() + 60, barY + 2, barMaxWidth - 60, 8), 3, 3);

        p.setBrush(entry.color);
        p.drawRoundedRect(QRect(rect.left() + 60, barY + 2, barW - 60, 8), 3, 3);

        p.setPen(entry.color);
        p.drawText(QRect(rect.right() - 55, y, 50, 18),
                   Qt::AlignRight | Qt::AlignVCenter,
                   QString("%1%").arg(entry.uptime * 100, 0, 'f', 1));

        y += 44;
    }

    if (entries_.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.drawText(rect, Qt::AlignCenter, "No endpoints checked");
    }
}

void PaperEndpointMonitor::drawCategoryChart(QPainter& p, const QRect& rect)
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
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Categories");

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

    QStringList categoryColors = {"#3b82f6", "#16a34a", "#d97706", "#dc2626", "#7c3aed", "#0ea5e9"};
    int y = rect.top() + 40;
    int barMaxWidth = rect.width() - 90;

    int colorIdx = 0;
    for (auto it = counts.constBegin(); it != counts.constEnd(); ++it) {
        if (y + 30 > rect.bottom()) break;

        p.setPen(QColor("#e2e8f0"));
        p.drawText(QRect(rect.left() + 10, y, 70, 24),
                   Qt::AlignLeft | Qt::AlignVCenter, it.key());

        int barW = maxCount > 0 ? static_cast<int>(barMaxWidth * (static_cast<qreal>(it.value()) / maxCount)) : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(categoryColors[colorIdx % categoryColors.size()]));
        p.drawRoundedRect(QRect(rect.left() + 85, y + 4, barW, 16), 3, 3);

        p.setPen(QColor("#94a3b8"));
        p.drawText(QRect(rect.left() + 85 + barW + 5, y, 30, 24),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   QString::number(it.value()));

        y += 30;
        colorIdx++;
    }
}

void PaperEndpointMonitor::drawStats(QPainter& p, const QRect& rect)
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
    p.drawText(rect.adjusted(10, 8, -10, 0), Qt::AlignLeft | Qt::AlignTop, "Statistics");

    QFont normalFont = font();
    p.setFont(normalFont);

    int y = rect.top() + 45;

    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, "Total Endpoints");
    p.setPen(QColor("#f8fafc"));
    p.drawText(QRect(rect.left() + 10, y + 20, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, QString::number(entries_.size()));

    y += 55;

    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, "Healthy");
    p.setPen(QColor("#16a34a"));
    p.drawText(QRect(rect.left() + 10, y + 20, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, QString::number(healthyCount()));

    y += 55;

    p.setPen(QColor("#94a3b8"));
    p.drawText(QRect(rect.left() + 10, y, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter, "Avg Uptime");
    p.setPen(QColor("#3b82f6"));
    p.drawText(QRect(rect.left() + 10, y + 20, rect.width() - 20, 20),
               Qt::AlignLeft | Qt::AlignVCenter,
               QString("%1%").arg(avgUptime() * 100, 0, 'f', 2));

    if (entries_.isEmpty()) {
        p.setPen(QColor("#64748b"));
        p.drawText(rect, Qt::AlignCenter, "No data");
    }
}

void PaperEndpointMonitor::updateInfo()
{
    int total = entries_.size();
    int healthy = healthyCount();
    qreal avg = avgUptime();
    infoLabel_->setText(QString("Endpoints: %1 | Healthy: %2 | Avg Uptime: %3%")
                            .arg(total)
                            .arg(healthy)
                            .arg(avg * 100, 0, 'f', 2));
}

void PaperEndpointMonitor::loadSettings()
{
    settings_.beginGroup("EndpointMonitor");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EndpointMonitorEntry entry;
        entry.id = settings_.value("id").toInt();
        entry.endpoint = settings_.value("endpoint").toString();
        entry.category = settings_.value("category").toString();
        entry.status = settings_.value("status").toString();
        entry.uptime = settings_.value("uptime").toReal();
        entry.errors = settings_.value("errors").toInt();
        entry.healthy = settings_.value("healthy").toBool();
        entry.color = QColor(settings_.value("color").toString());
        entries_.append(entry);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperEndpointMonitor::saveSettings()
{
    settings_.beginGroup("EndpointMonitor");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& entry = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id", entry.id);
        settings_.setValue("endpoint", entry.endpoint);
        settings_.setValue("category", entry.category);
        settings_.setValue("status", entry.status);
        settings_.setValue("uptime", entry.uptime);
        settings_.setValue("errors", entry.errors);
        settings_.setValue("healthy", entry.healthy);
        settings_.setValue("color", entry.color.name());
    }
    settings_.endArray();
    settings_.endGroup();
    settings_.sync();
}

void PaperEndpointMonitor::addEntry(const EndpointMonitorEntry& entry)
{
    entries_.append(entry);
    emit healthChecked(entry.id, entry.uptime);
}

QList<EndpointMonitorEntry> PaperEndpointMonitor::entries() const
{
    return entries_;
}

int PaperEndpointMonitor::healthyCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.healthy) ++count;
    }
    return count;
}

qreal PaperEndpointMonitor::avgUptime() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.uptime;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperEndpointMonitor::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}
