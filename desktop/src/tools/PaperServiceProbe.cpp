#include "tools/PaperServiceProbe.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperServiceProbe::PaperServiceProbe(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ServiceProbe")
{
    setupUI();
    loadSettings();
}

void PaperServiceProbe::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    probeBtn_ = new QPushButton("Probe");
    probeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(probeBtn_, &QPushButton::clicked, this, &PaperServiceProbe::onProbe);
    toolbar->addWidget(probeBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "API", "Database", "Cache", "Storage"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter service endpoint...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperServiceProbe::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Probe service health");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperServiceProbe::addEntry(const ProbeEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit probeComplete(entry.id, entry.latency);
    update();
}

QList<ProbeEntry> PaperServiceProbe::entries() const { return entries_; }

int PaperServiceProbe::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.healthy) c++;
    return c;
}

qreal PaperServiceProbe::avgLatency() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0.0;
    for (const auto& e : entries_) total += e.latency;
    return total / entries_.size();
}

QMap<QString, int> PaperServiceProbe::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperServiceProbe::onProbe() {
    QStringList categories = {"api", "database", "cache", "storage"};
    QStringList services = {"auth-svc", "paper-api", "search-engine", "crawler", "indexer",
                            "metadata-svc", "gateway", "scheduler"};
    QStringList endpoints = {"/health", "/status", "/ping", "/ready", "/metrics"};
    QList<QColor> palette = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6),
                             QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 4 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        ProbeEntry e;
        e.id = entries_.size() + 1;
        e.service = services[QRandomGenerator::global()->bounded(services.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())]
                                : categories[cIdx - 1];
        e.endpoint = endpoints[QRandomGenerator::global()->bounded(endpoints.size())];
        e.latency = 5.0 + QRandomGenerator::global()->bounded(495);
        e.checks = 1 + QRandomGenerator::global()->bounded(50);
        e.healthy = QRandomGenerator::global()->bounded(5) != 0;
        e.color = palette[QRandomGenerator::global()->bounded(palette.size())];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperServiceProbe::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Probe service health");
    update();
}

void PaperServiceProbe::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Probe service health");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Service Probe");
    int w = width(), h = height();
    drawProbeView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperServiceProbe::drawProbeView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(34, (rect.height() - 10) / qMax(show, 1));
    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 3);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(185));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.service + (e.healthy ? " [OK]" : " [DOWN]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.endpoint + " | " + QString::number(e.latency, 'f', 1) + "ms");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight, e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.checks) + " checks");
    }
}

void PaperServiceProbe::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"api", "database", "cache", "storage"};
    QString labels[] = {"API", "Database", "Cache", "Storage"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperServiceProbe::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Services", QString::number(entries_.size()), QColor(59,130,246)},
        {"Healthy", QString::number(healthyCount()), QColor(22,163,74)},
        {"Avg Latency", QString::number(avgLatency(), 'f', 1) + "ms", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
    };
    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperServiceProbe::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Probe service health"); return; }
    infoLabel_->setText(QString("%1 services | %2 healthy | %3ms avg")
        .arg(entries_.size()).arg(healthyCount()).arg(avgLatency(), 0, 'f', 1));
}

void PaperServiceProbe::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ProbeEntry e;
        e.id = settings_.value("id").toInt();
        e.service = settings_.value("service").toString();
        e.category = settings_.value("category").toString();
        e.endpoint = settings_.value("endpoint").toString();
        e.latency = settings_.value("latency").toReal();
        e.checks = settings_.value("checks").toInt();
        e.healthy = settings_.value("healthy").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperServiceProbe::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("service", entries_[i].service);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("endpoint", entries_[i].endpoint);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("checks", entries_[i].checks);
        settings_.setValue("healthy", entries_[i].healthy);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
