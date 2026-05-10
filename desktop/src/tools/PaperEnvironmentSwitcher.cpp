#include "tools/PaperEnvironmentSwitcher.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperEnvironmentSwitcher::PaperEnvironmentSwitcher(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "EnvironmentSwitcher")
{
    setupUI();
    loadSettings();
}

void PaperEnvironmentSwitcher::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    switchBtn_ = new QPushButton("Switch");
    switchBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(switchBtn_, &QPushButton::clicked, this, &PaperEnvironmentSwitcher::onSwitch);
    toolbar->addWidget(switchBtn_);
    toolbar->addWidget(new QLabel("Region:"));
    regionCombo_ = new QComboBox();
    regionCombo_->addItems({"All", "US-East", "EU-West", "Asia"});
    toolbar->addWidget(regionCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperEnvironmentSwitcher::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter environment name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Switch environments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperEnvironmentSwitcher::addEntry(const EnvironmentEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit environmentSwitched(entry.id, entry.latency);
    update();
}

QList<EnvironmentEntry> PaperEnvironmentSwitcher::entries() const { return entries_; }

qreal PaperEnvironmentSwitcher::avgLatency() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.latency;
    return sum / entries_.size();
}

int PaperEnvironmentSwitcher::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.healthy) c++;
    return c;
}

QMap<QString, int> PaperEnvironmentSwitcher::regionCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.region]++;
    return counts;
}

void PaperEnvironmentSwitcher::onSwitch() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList regions = {"us-east", "eu-west", "asia"};
    QStringList versions = {"v1", "v2", "v3"};
    QStringList suffixes = {"-prod", "-staging", "-dev"};
    int rIdx = regionCombo_->currentIndex();
    EnvironmentEntry e;
    e.id = entries_.size() + 1;
    e.envName = text.left(10).toLower() + suffixes[QRandomGenerator::global()->bounded(suffixes.size())];
    int regIdx = rIdx == 0 ? QRandomGenerator::global()->bounded(regions.size()) : rIdx - 1;
    e.region = regions[regIdx];
    e.baseUrl = "https://" + e.envName + "." + e.region + ".api";
    e.latency = 10 + QRandomGenerator::global()->bounded(200);
    e.healthy = e.latency < 100;
    e.papersCached = QRandomGenerator::global()->bounded(10000);
    e.apiVersion = versions[QRandomGenerator::global()->bounded(versions.size())];
    e.lastChecked = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(10));
    e.active = e.healthy;
    e.color = e.healthy ? QColor(16,185,129) : (e.latency < 150 ? QColor(245,158,11) : QColor(239,68,68));
    addEntry(e);
    inputField_->clear();
}

void PaperEnvironmentSwitcher::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Switch environments");
    update();
}

void PaperEnvironmentSwitcher::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Switch environments");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Environment Switcher");
    int w = width(), h = height();
    drawEnvList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawRegionChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperEnvironmentSwitcher::drawEnvList(QPainter& p, const QRect& rect) {
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
        p.setFont(QFont("Courier", 8, QFont::Bold));
        p.drawText(rect.x() + 10, y + 4, rect.width() / 2 - 10, 16, Qt::AlignVCenter,
                   e.envName.left(16) + (e.active ? " [ON]" : " [OFF]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.region + " | " + e.apiVersion + " | " + QString::number(e.papersCached) + " cached");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.latency) + "ms");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.lastChecked);
    }
}

void PaperEnvironmentSwitcher::drawRegionChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Regions");
    auto counts = regionCounts();
    QStringList regions = {"us-east", "eu-west", "asia"};
    QString labels[] = {"US-East", "EU-West", "Asia"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(regions[i]) ? counts[regions[i]] : 0;
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

void PaperEnvironmentSwitcher::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Environments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Healthy", QString::number(healthyCount()), QColor(16,185,129)},
        {"Avg Latency", QString::number(avgLatency(), 'f', 0) + "ms", QColor(245,158,11)},
        {"Regions", QString::number(regionCounts().size()), QColor(139,92,246)}
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

void PaperEnvironmentSwitcher::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Switch environments"); return; }
    infoLabel_->setText(QString("%1 envs | %2 healthy | %3ms avg")
        .arg(entries_.size()).arg(healthyCount()).arg(avgLatency(), 0, 'f', 0));
}

void PaperEnvironmentSwitcher::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        EnvironmentEntry e;
        e.id = settings_.value("id").toInt();
        e.envName = settings_.value("envName").toString();
        e.baseUrl = settings_.value("baseUrl").toString();
        e.region = settings_.value("region").toString();
        e.latency = settings_.value("latency").toInt();
        e.healthy = settings_.value("healthy").toBool();
        e.papersCached = settings_.value("papersCached").toInt();
        e.apiVersion = settings_.value("apiVersion").toString();
        e.lastChecked = settings_.value("lastChecked").toString();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperEnvironmentSwitcher::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("envName", entries_[i].envName);
        settings_.setValue("baseUrl", entries_[i].baseUrl);
        settings_.setValue("region", entries_[i].region);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("healthy", entries_[i].healthy);
        settings_.setValue("papersCached", entries_[i].papersCached);
        settings_.setValue("apiVersion", entries_[i].apiVersion);
        settings_.setValue("lastChecked", entries_[i].lastChecked);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
