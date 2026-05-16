#include "tools/PaperMetricExporter2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperMetricExporter2::PaperMetricExporter2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "MetricExporter2")
{
    setupUI();
    loadSettings();
}

void PaperMetricExporter2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(0, 0, 0, 4);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Application", "Infrastructure", "Network", "Database", "Security"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "min-width: 120px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search metrics...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    exportBtn_ = new QPushButton("Export");
    exportBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperMetricExporter2::onExport);
    toolbar->addWidget(exportBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #fca5a5; "
        "border-radius: 4px; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperMetricExporter2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Metric Exporter ready");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #64748b; padding: 2px 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(720, 520);
}

void PaperMetricExporter2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No metrics exported yet");
        return;
    }

    int w = width(), h = height();
    int toolbarH = 70;

    drawExporterView(p, QRect(0, toolbarH, static_cast<int>(w * 0.6), h - toolbarH - static_cast<int>(h * 0.25)));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6), toolbarH, w - static_cast<int>(w * 0.6), h - toolbarH - static_cast<int>(h * 0.25)));
    drawStats(p, QRect(0, h - static_cast<int>(h * 0.25), w, static_cast<int>(h * 0.25)));
}

void PaperMetricExporter2::drawExporterView(QPainter& p, const QRect& rect) {
    int margin = 12;
    int x = rect.x() + margin;
    int y = rect.y() + margin;
    int viewW = rect.width() - margin * 2;
    int viewH = rect.height() - margin * 2;

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(x, y, viewW, 22, Qt::AlignLeft | Qt::AlignVCenter, "Metric Cards");

    y += 28;

    int show = qMin(8, entries_.size());
    int cardH = qMin(48, (viewH - 28) / qMax(show, 1) - 4);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int cardY = y + i * (cardH + 4);

        if (cardY + cardH > rect.y() + rect.height()) break;

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(x, cardY, viewW, cardH, 6, 6);

        // Left color accent
        p.setBrush(e.color);
        p.drawRoundedRect(x, cardY, 4, cardH, 2, 2);

        // Format badge
        QColor badgeColor = e.color;
        p.setBrush(badgeColor);
        QFontMetrics fm(QFont("Arial", 7));
        int badgeW = fm.horizontalAdvance(e.format) + 12;
        p.drawRoundedRect(x + 10, cardY + 5, badgeW, 16, 3, 3);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(x + 10, cardY + 5, badgeW, 16, Qt::AlignCenter, e.format);

        // Metric name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(x + 16 + badgeW, cardY + 3, viewW - badgeW - 80, 18,
                   Qt::AlignVCenter, e.metric.left(24));

        // Category label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x + 16 + badgeW, cardY + 20, viewW - badgeW - 80, 16,
                   Qt::AlignVCenter, e.category);

        // Volume bar
        int barX = x + viewW - 120;
        int barW = 70;
        int barH = 8;
        int barY = cardY + 10;

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, barH, 3, 3);

        qreal maxVol = 1000.0;
        int fillW = static_cast<int>((qMin(e.volume, maxVol) / maxVol) * barW);
        p.setBrush(e.color);
        p.drawRoundedRect(barX, barY, fillW, barH, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX, barY + barH + 1, barW, 14, Qt::AlignCenter,
                   QString::number(e.volume, 'f', 0));

        // Export count
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(barX + barW + 5, cardY + 10, 40, 16, Qt::AlignVCenter,
                   QString::number(e.exports) + " exp");

        // Active indicator
        if (e.active) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(34, 197, 94));
            p.drawEllipse(x + viewW - 16, cardY + cardH / 2 - 4, 8, 8);
        }
    }
}

void PaperMetricExporter2::drawCategoryChart(QPainter& p, const QRect& rect) {
    int margin = 12;
    int x = rect.x() + margin;
    int y = rect.y() + margin;
    int chartW = rect.width() - margin * 2;
    int chartH = rect.height() - margin * 2;

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(x, y, chartW, 22, Qt::AlignLeft | Qt::AlignVCenter, "Format Distribution");
    y += 28;

    // Count entries per format
    QMap<QString, int> formatCounts;
    QStringList formats = {"JSON", "CSV", "Prometheus", "InfluxDB", "OpenTelemetry"};
    for (const auto& e : entries_) {
        formatCounts[e.format]++;
    }

    int maxVal = 1;
    for (const auto& f : formats) {
        maxVal = qMax(maxVal, formatCounts.value(f, 0));
    }

    QColor colors[] = {
        QColor(59, 130, 246),   // JSON - #3b82f6
        QColor(22, 163, 74),    // CSV - #16a34a
        QColor(217, 119, 6),    // Prometheus - #d97706
        QColor(220, 38, 38),    // InfluxDB - #dc2626
        QColor(124, 58, 237)    // OpenTelemetry - #7c3aed
    };

    int barH = qMin(24, (chartH - 28) / 5 - 4);
    int labelW = 90;

    for (int i = 0; i < 5; ++i) {
        int barY = y + i * (barH + 6);
        int count = formatCounts.value(formats[i], 0);
        int barFillW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (chartW - labelW - 30));

        // Format label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(x, barY, labelW, barH, Qt::AlignRight | Qt::AlignVCenter, formats[i]);

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(x + labelW + 6, barY + 2, chartW - labelW - 30, barH - 4, 3, 3);

        // Bar fill
        if (barFillW > 0) {
            p.setBrush(colors[i]);
            p.drawRoundedRect(x + labelW + 6, barY + 2, barFillW, barH - 4, 3, 3);
        }

        // Count text
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x + labelW + 10 + barFillW, barY, 30, barH, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperMetricExporter2::drawStats(QPainter& p, const QRect& rect) {
    int margin = 12;
    int y = rect.y() + margin;
    int boxH = qMin(50, (rect.height() - margin * 2) - 10);
    int totalW = rect.width() - margin * 2;
    int boxW = (totalW - 30) / 4;

    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Total Metrics",  QString::number(entries_.size()),  QColor(59, 130, 246)},
        {"Active Count",   QString::number(activeCount()),    QColor(22, 163, 74)},
        {"Avg Volume",     QString::number(avgVolume(), 'f', 1), QColor(217, 119, 6)},
        {"Total Exports",  QString::number([](const QList<MetricExporter2Entry>& es) {
                              int t = 0; for (const auto& e : es) t += e.exports; return t;
                          }(entries_)),                       QColor(124, 58, 237)}
    };

    for (int i = 0; i < stats.size(); ++i) {
        int bx = rect.x() + margin + i * (boxW + 10);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(192));
        p.drawRoundedRect(bx, y, boxW, boxH, 6, 6);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, y, boxW, 3, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 15, QFont::Bold));
        p.drawText(bx + 8, y + 6, boxW - 16, 24, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(bx + 8, y + 30, boxW - 16, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperMetricExporter2::onExport() {
    static const QStringList metrics = {
        "Request Count", "Error Rate", "Latency P99", "Throughput"
    };
    static const QStringList categories = {
        "Application", "Infrastructure", "Network", "Database", "Security"
    };
    static const QStringList formats = {
        "JSON", "CSV", "Prometheus", "InfluxDB", "OpenTelemetry"
    };
    static const QColor colors[] = {
        QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
        QColor(220, 38, 38), QColor(124, 58, 237)
    };

    MetricExporter2Entry e;
    e.id = entries_.size() + 1;
    e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.format = formats[QRandomGenerator::global()->bounded(formats.size())];
    e.volume = QRandomGenerator::global()->bounded(100000) / 100.0;
    e.exports = QRandomGenerator::global()->bounded(50);
    e.active = QRandomGenerator::global()->bounded(2) == 0;
    e.color = colors[QRandomGenerator::global()->bounded(5)];

    addEntry(e);
}

void PaperMetricExporter2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperMetricExporter2::addEntry(const MetricExporter2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit exportComplete(entry.id, entry.volume);
    update();
}

QList<MetricExporter2Entry> PaperMetricExporter2::entries() const {
    return entries_;
}

int PaperMetricExporter2::activeCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.active) c++;
    return c;
}

qreal PaperMetricExporter2::avgVolume() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_)
        sum += e.volume;
    return sum / entries_.size();
}

QMap<QString, int> PaperMetricExporter2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_)
        counts[e.category]++;
    return counts;
}

void PaperMetricExporter2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Metric Exporter ready");
        return;
    }
    infoLabel_->setText(QString("%1 metrics | %2 active | avg vol %3 | %4 exports")
        .arg(entries_.size())
        .arg(activeCount())
        .arg(avgVolume(), 0, 'f', 1)
        .arg([](const QList<MetricExporter2Entry>& es) {
            int t = 0; for (const auto& e : es) t += e.exports; return t;
        }(entries_)));
}

void PaperMetricExporter2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        MetricExporter2Entry e;
        e.id = settings_.value("id").toInt();
        e.metric = settings_.value("metric").toString();
        e.category = settings_.value("category").toString();
        e.format = settings_.value("format").toString();
        e.volume = settings_.value("volume").toDouble();
        e.exports = settings_.value("exports").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();

    if (entries_.isEmpty()) {
        // Seed 8 demo entries
        static const QStringList metrics = {
            "Request Count", "Error Rate", "Latency P99", "Throughput"
        };
        static const QStringList categories = {
            "Application", "Infrastructure", "Network", "Database", "Security"
        };
        static const QStringList formats = {
            "JSON", "CSV", "Prometheus", "InfluxDB", "OpenTelemetry"
        };
        static const QColor colors[] = {
            QColor(59, 130, 246), QColor(22, 163, 74), QColor(217, 119, 6),
            QColor(220, 38, 38), QColor(124, 58, 237)
        };

        for (int i = 0; i < 8; ++i) {
            MetricExporter2Entry e;
            e.id = i + 1;
            e.metric = metrics[i % metrics.size()];
            e.category = categories[i % categories.size()];
            e.format = formats[i % formats.size()];
            e.volume = 50.0 + i * 120.5;
            e.exports = 5 + i * 3;
            e.active = (i % 3) != 1;
            e.color = colors[i % 5];
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperMetricExporter2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("format", entries_[i].format);
        settings_.setValue("volume", entries_[i].volume);
        settings_.setValue("exports", entries_[i].exports);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
