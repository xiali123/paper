#include "tools/PaperApiDebugger.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QPainterPath>
#include <QtMath>

PaperApiDebugger::PaperApiDebugger(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ApiDebugger")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList endpoints = {"/api/papers", "/api/search", "/api/auth", "/api/citations"};
        QStringList methods = {"GET", "POST", "PUT", "DELETE"};
        QStringList categories = {"REST", "GraphQL", "WebSocket", "gRPC", "Internal"};
        QColor palette[] = {
            QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
            QColor("#dc2626"), QColor("#7c3aed")
        };

        for (int i = 0; i < 8; ++i) {
            ApiDebuggerEntry e;
            e.id = i + 1;
            e.endpoint = endpoints[i % endpoints.size()];
            e.method = methods[i % methods.size()];
            e.category = categories[i % categories.size()];
            e.latency = 20.0 + QRandomGenerator::global()->bounded(980);
            e.requests = 5 + QRandomGenerator::global()->bounded(200);
            e.failing = QRandomGenerator::global()->bounded(4) == 0;
            e.color = palette[i % 5];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperApiDebugger::setupUI()
{
    auto* layout = new QHBoxLayout(this);

    categoryCombo_ = new QComboBox(this);
    categoryCombo_->addItems({"All", "REST", "GraphQL", "WebSocket", "gRPC", "Internal"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 90px; }");
    layout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit(this);
    inputField_->setPlaceholderText("Search endpoints...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_, 1);

    debugBtn_ = new QPushButton("Debug", this);
    debugBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(debugBtn_, &QPushButton::clicked, this, &PaperApiDebugger::onDebug);
    layout->addWidget(debugBtn_);

    clearBtn_ = new QPushButton("Clear", this);
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 4px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperApiDebugger::onClear);
    layout->addWidget(clearBtn_);

    layout->addStretch();

    infoLabel_ = new QLabel(this);
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px 8px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(720, 520);
}

void PaperApiDebugger::addEntry(const ApiDebuggerEntry& entry)
{
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit endpointTested(entry.id, entry.latency);
    update();
}

QList<ApiDebuggerEntry> PaperApiDebugger::entries() const
{
    return entries_;
}

int PaperApiDebugger::failingCount() const
{
    int count = 0;
    for (const auto& e : entries_) {
        if (e.failing) ++count;
    }
    return count;
}

qreal PaperApiDebugger::avgLatency() const
{
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) {
        sum += e.latency;
    }
    return sum / entries_.size();
}

QMap<QString, int> PaperApiDebugger::categoryCounts() const
{
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperApiDebugger::onDebug()
{
    QStringList endpoints = {"/api/papers", "/api/search", "/api/auth", "/api/citations"};
    QStringList methods = {"GET", "POST", "PUT", "DELETE"};
    QStringList categories = {"REST", "GraphQL", "WebSocket", "gRPC", "Internal"};
    QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    ApiDebuggerEntry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.endpoint = endpoints[QRandomGenerator::global()->bounded(endpoints.size())];
    e.method = methods[QRandomGenerator::global()->bounded(methods.size())];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.latency = 10.0 + QRandomGenerator::global()->bounded(1990);
    e.requests = 1 + QRandomGenerator::global()->bounded(300);
    e.failing = QRandomGenerator::global()->bounded(5) == 0;
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperApiDebugger::onClear()
{
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperApiDebugger::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No API endpoints - click Debug to add");
        return;
    }

    int w = width();
    int h = height();
    int topH = static_cast<int>(h * 0.75);
    int bottomH = h - topH;

    int leftW = static_cast<int>(w * 0.6);
    int rightW = w - leftW;

    drawDebuggerView(p, QRect(0, 0, leftW, topH));
    drawCategoryChart(p, QRect(leftW, 0, rightW, topH));
    drawStats(p, QRect(0, topH, w, bottomH));
}

void PaperApiDebugger::drawDebuggerView(QPainter& p, const QRect& rect)
{
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.adjusted(16, 12, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               "API Debugger");

    // Filter by current combo selection
    QString filter = categoryCombo_->currentText();
    QString search = inputField_->text().trimmed().toLower();

    QList<const ApiDebuggerEntry*> visible;
    for (const auto& e : entries_) {
        if (filter != "All" && e.category != filter) continue;
        if (!search.isEmpty() && !e.endpoint.toLower().contains(search)) continue;
        visible.append(&e);
    }

    if (visible.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect.adjusted(16, 50, -16, 0), Qt::AlignLeft | Qt::AlignTop,
                   "No matching endpoints");
        return;
    }

    int margin = 16;
    int topPad = 42;
    int rowH = 48;
    int gap = 4;
    int listX = rect.x() + margin;
    int listW = rect.width() - 2 * margin;
    int maxRows = qMin(visible.size(), (rect.height() - topPad - margin) / (rowH + gap));

    // Method color map
    QMap<QString, QColor> methodColor;
    methodColor["GET"] = QColor("#16a34a");
    methodColor["POST"] = QColor("#3b82f6");
    methodColor["PUT"] = QColor("#d97706");
    methodColor["DELETE"] = QColor("#dc2626");

    for (int i = 0; i < maxRows; ++i) {
        const auto& e = *visible[i];
        int y = rect.y() + topPad + i * (rowH + gap);

        // Row background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(listX, y, listW, rowH, 6, 6);

        // Left accent bar
        p.setBrush(e.color);
        p.drawRoundedRect(listX, y, 4, rowH, 2, 2);

        // Method badge
        QColor badgeColor = methodColor.value(e.method, QColor("#7c3aed"));
        int badgeW = 56;
        int badgeH = 22;
        int badgeX = listX + 12;
        int badgeY = y + (rowH - badgeH) / 2;

        p.setBrush(badgeColor);
        p.drawRoundedRect(badgeX, badgeY, badgeW, badgeH, 4, 4);

        p.setPen(Qt::white);
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(QRect(badgeX, badgeY, badgeW, badgeH),
                   Qt::AlignCenter, e.method);

        // Endpoint path
        int pathX = badgeX + badgeW + 10;
        int pathW = listW * 0.35;
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Courier", 9));
        p.drawText(QRect(pathX, y, pathW, rowH),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   e.endpoint);

        // Latency bar
        int barX = pathX + pathW + 8;
        int barMaxW = listW * 0.2;
        int barH2 = 10;
        int barY2 = y + rowH / 2 - barH2 / 2;

        qreal latencyNorm = qMin(e.latency / 2000.0, 1.0);
        int barW2 = static_cast<int>(barMaxW * latencyNorm);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY2, barMaxW, barH2, 3, 3);

        QColor latencyColor = e.latency < 200 ? QColor("#16a34a") :
                              (e.latency < 800 ? QColor("#d97706") : QColor("#dc2626"));
        p.setBrush(latencyColor);
        p.drawRoundedRect(barX, barY2, barW2, barH2, 3, 3);

        // Latency label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(QRect(barX + barMaxW + 4, y, 50, rowH),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(static_cast<int>(e.latency)) + "ms");

        // Request count
        int reqX = barX + barMaxW + 56;
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(QRect(reqX, y, 50, rowH / 2),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   QString::number(e.requests) + " req");

        // Category label
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 7));
        p.drawText(QRect(reqX, y + rowH / 2, 70, rowH / 2),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   e.category);

        // Failing indicator
        if (e.failing) {
            int indX = listX + listW - 30;
            p.setPen(Qt::NoPen);
            p.setBrush(QColor("#dc2626"));
            p.drawEllipse(indX, y + rowH / 2 - 6, 12, 12);
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(QRect(indX, y + rowH / 2 - 6, 12, 12),
                       Qt::AlignCenter, "!");
        }
    }
}

void PaperApiDebugger::drawCategoryChart(QPainter& p, const QRect& rect)
{
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.adjusted(16, 12, -10, 0), Qt::AlignLeft | Qt::AlignTop,
               "Method Distribution");

    // Count by method
    QMap<QString, int> methodCounts;
    for (const auto& e : entries_) {
        methodCounts[e.method]++;
    }

    if (methodCounts.isEmpty()) {
        p.setPen(QColor(148, 163, 184));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No data");
        return;
    }

    int total = 0;
    for (auto it = methodCounts.constBegin(); it != methodCounts.constEnd(); ++it) {
        total += it.value();
    }

    QMap<QString, QColor> methodColor;
    methodColor["GET"] = QColor("#16a34a");
    methodColor["POST"] = QColor("#3b82f6");
    methodColor["PUT"] = QColor("#d97706");
    methodColor["DELETE"] = QColor("#dc2626");

    // Pie chart
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 40 + (rect.height() - 100) / 2;
    int radius = qMin(rect.width() - 40, rect.height() - 120) / 2;
    radius = qMax(radius, 40);

    qreal startAngle = 0.0;
    int legendY = cy + radius + 16;

    for (auto it = methodCounts.constBegin(); it != methodCounts.constEnd(); ++it) {
        qreal span = (static_cast<qreal>(it.value()) / total) * 360.0 * 16.0;
        QColor color = methodColor.value(it.key(), QColor("#7c3aed"));

        p.setPen(Qt::NoPen);
        p.setBrush(color);
        p.drawPie(cx - radius, cy - radius, radius * 2, radius * 2,
                  static_cast<int>(startAngle), static_cast<int>(span));

        // Legend entry
        int legendX = rect.x() + 20;
        if (legendY + 16 > rect.bottom() - 4) break;

        p.setBrush(color);
        p.drawRoundedRect(legendX, legendY, 10, 10, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        qreal pct = (static_cast<qreal>(it.value()) / total) * 100.0;
        p.drawText(legendX + 16, legendY + 10,
                   QString("%1 (%2, %3%)")
                       .arg(it.key())
                       .arg(it.value())
                       .arg(pct, 0, 'f', 0));

        startAngle += span;
        legendY += 20;
    }
}

void PaperApiDebugger::drawStats(QPainter& p, const QRect& rect)
{
    int totalRequests = 0;
    for (const auto& e : entries_) {
        totalRequests += e.requests;
    }

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };
    QList<Stat> stats = {
        {"Total Endpoints", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Failing",         QString::number(failingCount()), QColor("#dc2626")},
        {"Avg Latency",     QString::number(avgLatency(), 'f', 0) + "ms", QColor("#d97706")},
        {"Total Requests",  QString::number(totalRequests), QColor("#16a34a")}
    };

    int margin = 12;
    int gap = 12;
    int boxW = (rect.width() - 2 * margin - (stats.size() - 1) * gap) / stats.size();
    int boxH = rect.height() - 2 * margin;
    int topY = rect.y() + margin;

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + margin + i * (boxW + gap);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, topY, boxW, boxH, 8, 8);

        // Top accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, topY, boxW, 4, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(QRect(x + 12, topY + 10, boxW - 24, boxH / 2),
                   Qt::AlignLeft | Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(x + 12, topY + boxH / 2 + 4, boxW - 24, boxH / 2 - 14),
                   Qt::AlignLeft | Qt::AlignVCenter, stats[i].label);
    }
}

void PaperApiDebugger::updateInfo()
{
    if (entries_.isEmpty()) {
        infoLabel_->setText("No endpoints");
        return;
    }
    infoLabel_->setText(
        QString("%1 endpoints | %2 failing | %3ms avg")
            .arg(entries_.size())
            .arg(failingCount())
            .arg(avgLatency(), 0, 'f', 0));
}

void PaperApiDebugger::loadSettings()
{
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ApiDebuggerEntry e;
        e.id = settings_.value("id").toInt();
        e.endpoint = settings_.value("endpoint").toString();
        e.category = settings_.value("category").toString();
        e.method = settings_.value("method").toString();
        e.latency = settings_.value("latency").toReal();
        e.requests = settings_.value("requests").toInt();
        e.failing = settings_.value("failing").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperApiDebugger::saveSettings()
{
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setArrayIndex(i);
        settings_.setValue("id", e.id);
        settings_.setValue("endpoint", e.endpoint);
        settings_.setValue("category", e.category);
        settings_.setValue("method", e.method);
        settings_.setValue("latency", e.latency);
        settings_.setValue("requests", e.requests);
        settings_.setValue("failing", e.failing);
        settings_.setValue("color", e.color.name());
    }
    settings_.endArray();
    settings_.sync();
}
