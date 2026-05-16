#include "tools/PaperLatencyProfiler.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperLatencyProfiler::PaperLatencyProfiler(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LatencyProfiler")
{
    setupUI();
    loadSettings();
}

void PaperLatencyProfiler::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left panel
    auto* leftPanel = new QVBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"search", "paper", "citation", "user", "analytics"});
    leftPanel->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Endpoint URL...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftPanel->addWidget(inputField_);

    profileBtn_ = new QPushButton("Profile");
    profileBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(profileBtn_, &QPushButton::clicked, this, &PaperLatencyProfiler::onProfile);
    leftPanel->addWidget(profileBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("QPushButton { color: #dc2626; padding: 4px 12px; border-radius: 4px; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLatencyProfiler::onClear);
    leftPanel->addWidget(clearBtn_);

    leftPanel->addStretch();

    infoLabel_ = new QLabel("Latency Profiler");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftPanel->addWidget(infoLabel_);

    mainLayout->addLayout(leftPanel, 1);

    // Right area reserved for custom painting
    mainLayout->addStretch(3);

    setMinimumSize(700, 480);
}

void PaperLatencyProfiler::addEntry(const LatencyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<LatencyEntry> PaperLatencyProfiler::entries() const {
    return entries_;
}

int PaperLatencyProfiler::slowCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.slow) ++c;
    return c;
}

qreal PaperLatencyProfiler::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.latency;
    return sum / entries_.size();
}

QMap<QString, int> PaperLatencyProfiler::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLatencyProfiler::onProfile() {
    QString endpoint = inputField_->text().trimmed();
    if (endpoint.isEmpty()) return;

    static const QColor palette[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    QStringList methods = {"GET", "POST", "PUT", "DELETE", "PATCH"};

    LatencyEntry entry;
    entry.id       = entries_.size() + 1;
    entry.endpoint = endpoint;
    entry.category = categoryCombo_->currentText();
    entry.method   = methods[QRandomGenerator::global()->bounded(methods.size())];
    entry.latency  = 1.0 + QRandomGenerator::global()->bounded(500);
    entry.calls    = 1 + QRandomGenerator::global()->bounded(1000);
    entry.slow     = entry.latency > 200.0;
    entry.color    = palette[QRandomGenerator::global()->bounded(5)];

    addEntry(entry);
    emit profileComplete(entry.id, entry.latency);
    inputField_->clear();
}

void PaperLatencyProfiler::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperLatencyProfiler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Latency Profiler");
        return;
    }

    int w = width(), h = height();

    // Horizontal three-column layout for charts
    int colW = (w - 80) / 3;
    drawLatencyChart(p, QRect(20, 50, colW, h - 80));
    drawCategoryChart(p, QRect(30 + colW, 50, colW, h - 80));
    drawStats(p, QRect(40 + 2 * colW, 50, colW, h - 80));
}

void PaperLatencyProfiler::drawLatencyChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "API Latency");

    if (entries_.isEmpty()) return;

    int show = qMin(20, entries_.size());
    qreal maxLat = 1.0;
    for (int i = 0; i < show; ++i)
        maxLat = qMax(maxLat, entries_[i].latency);

    int chartTop = rect.y() + 30;
    int chartH   = rect.height() - 50;
    int barW     = qMax(6, (rect.width() - 10) / show - 2);

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int barH = static_cast<int>((e.latency / maxLat) * chartH);
        int x    = rect.x() + 5 + i * (barW + 2);
        int y    = chartTop + chartH - barH;

        // Color: slow=red, fast=green
        QColor barColor = e.slow ? QColor(220, 38, 38) : QColor(22, 163, 74);

        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(x, y, barW, barH, 2, 2);

        // Endpoint label (rotated or truncated)
        if (barW >= 14) {
            p.save();
            p.translate(x + barW / 2, chartTop + chartH + 4);
            p.rotate(-45);
            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 6));
            p.drawText(0, 0, e.endpoint.left(12));
            p.restore();
        }
    }

    // Y-axis label
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 7));
    p.drawText(rect.x(), chartTop - 2, QString("%1 ms").arg(maxLat, 0, 'f', 0));
    p.drawText(rect.x(), chartTop + chartH + 2, "0 ms");
}

void PaperLatencyProfiler::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y(), rect.width(), 24, Qt::AlignLeft | Qt::AlignVCenter,
               "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"search", "paper", "citation", "user", "analytics"};
    QString labels[] = {"Search", "Paper", "Citation", "User", "Analytics"};
    QColor colors[] = {
        QColor(59, 130, 246),   // #3b82f6
        QColor(22, 163, 74),    // #16a34a
        QColor(217, 119, 6),    // #d97706
        QColor(220, 38, 38),    // #dc2626
        QColor(124, 58, 237)    // #7c3aed
    };

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int chartTop = rect.y() + 34;
    int barH     = qMin(24, (rect.height() - 50) / 5);
    int maxBarW  = rect.width() - 100;

    for (int i = 0; i < 5; ++i) {
        int y     = chartTop + i * (barH + 6);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW  = static_cast<int>((static_cast<qreal>(count) / maxVal) * maxBarW);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        // Bar
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y + 2, barW, barH - 4, 3, 3);

        // Count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + 64 + barW, y + 2, 30, barH - 4, Qt::AlignVCenter,
                   QString::number(count));
    }
}

void PaperLatencyProfiler::drawStats(QPainter& p, const QRect& rect) {
    struct Stat {
        QString label;
        QString value;
        QColor color;
    };

    QList<Stat> stats = {
        {"Endpoints",  QString::number(entries_.size()),  QColor(59, 130, 246)},
        {"Slow",       QString::number(slowCount()),      QColor(220, 38, 38)},
        {"Avg Latency", QString::number(avgLatency(), 'f', 1) + " ms", QColor(217, 119, 6)}
    };

    int boxH = qMin(60, (rect.height() - 20) / 3);

    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(rect.x() + 12, y + 6, rect.width() - 24, boxH / 2, Qt::AlignVCenter,
                   stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 12, y + boxH / 2, rect.width() - 24, boxH / 2 - 4,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperLatencyProfiler::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Latency Profiler");
        return;
    }
    infoLabel_->setText(QString("Endpoints: %1 | Slow: %2 | Avg: %3 ms")
        .arg(entries_.size())
        .arg(slowCount())
        .arg(avgLatency(), 0, 'f', 1));
}

void PaperLatencyProfiler::loadSettings() {
    settings_.beginGroup("LatencyProfiler");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LatencyEntry e;
        e.id       = settings_.value("id").toInt();
        e.endpoint = settings_.value("endpoint").toString();
        e.category = settings_.value("category").toString();
        e.method   = settings_.value("method").toString();
        e.latency  = settings_.value("latency").toDouble();
        e.calls    = settings_.value("calls").toInt();
        e.slow     = settings_.value("slow").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperLatencyProfiler::saveSettings() {
    settings_.beginGroup("LatencyProfiler");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("endpoint", entries_[i].endpoint);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("method",   entries_[i].method);
        settings_.setValue("latency",  entries_[i].latency);
        settings_.setValue("calls",    entries_[i].calls);
        settings_.setValue("slow",     entries_[i].slow);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
