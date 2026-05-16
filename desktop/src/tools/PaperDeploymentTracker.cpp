#include "tools/PaperDeploymentTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDeploymentTracker::PaperDeploymentTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DeploymentTracker")
{
    setupUI();
    loadSettings();
}

void PaperDeploymentTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    deployBtn_ = new QPushButton("Deploy");
    deployBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(deployBtn_, &QPushButton::clicked, this, &PaperDeploymentTracker::onDeploy);
    toolbar->addWidget(deployBtn_);
    toolbar->addWidget(new QLabel("Env:"));
    envCombo_ = new QComboBox();
    envCombo_->addItems({"All", "Production", "Staging", "Development"});
    toolbar->addWidget(envCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDeploymentTracker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter app name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Track deployments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperDeploymentTracker::addEntry(const DeploymentEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit deploymentTracked(entry.id, entry.uptime);
    update();
}

QList<DeploymentEntry> PaperDeploymentTracker::entries() const { return entries_; }

int PaperDeploymentTracker::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.healthy) c++;
    return c;
}

qreal PaperDeploymentTracker::avgUptime() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.uptime;
    return sum / entries_.size();
}

QMap<QString, int> PaperDeploymentTracker::envCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.environment]++;
    return counts;
}

void PaperDeploymentTracker::onDeploy() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList envs = {"production", "staging", "development"};
    QStringList statuses = {"running", "deploying", "stopped", "error"};
    int eIdx = envCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        DeploymentEntry e;
        e.id = entries_.size() + 1;
        e.appName = text.left(10).toLower() + "-app-" + QString::number(i);
        e.environment = eIdx == 0 ? envs[QRandomGenerator::global()->bounded(envs.size())] : envs[eIdx - 1];
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.version = "v" + QString::number(QRandomGenerator::global()->bounded(5)) + "."
                    + QString::number(QRandomGenerator::global()->bounded(20)) + "."
                    + QString::number(QRandomGenerator::global()->bounded(100));
        e.uptime = 90 + QRandomGenerator::global()->bounded(1000) / 100.0;
        e.requests = 100 + QRandomGenerator::global()->bounded(100000);
        e.latency = 5 + QRandomGenerator::global()->bounded(500);
        e.healthy = e.status == "running" && e.uptime >= 99.0;
        e.color = e.healthy ? QColor(16,185,129) : (e.status == "deploying" ? QColor(59,130,246) : QColor(239,68,68));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperDeploymentTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track deployments");
    update();
}

void PaperDeploymentTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track deployments");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Deployment Tracker");
    int w = width(), h = height();
    drawDeploymentList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawEnvChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDeploymentTracker::drawDeploymentList(QPainter& p, const QRect& rect) {
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
                   e.appName.left(16) + (e.healthy ? " [OK]" : " [!!]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.environment + " | " + e.status + " | " + e.version);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.uptime, 'f', 2) + "% up");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.requests) + " req | " + QString::number(e.latency) + "ms");
    }
}

void PaperDeploymentTracker::drawEnvChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Environments");
    auto counts = envCounts();
    QStringList envs = {"production", "staging", "development"};
    QString labels[] = {"Production", "Staging", "Development"};
    QColor colors[] = {QColor(239,68,68), QColor(245,158,11), QColor(59,130,246)};
    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;
    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(envs[i]) ? counts[envs[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }
    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperDeploymentTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Apps", QString::number(entries_.size()), QColor(59,130,246)},
        {"Healthy", QString::number(healthyCount()), QColor(16,185,129)},
        {"Avg Uptime", QString::number(avgUptime(), 'f', 1) + "%", QColor(245,158,11)},
        {"Envs", QString::number(envCounts().size()), QColor(139,92,246)}
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

void PaperDeploymentTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track deployments"); return; }
    infoLabel_->setText(QString("%1 apps | %2 healthy | %3% uptime")
        .arg(entries_.size()).arg(healthyCount()).arg(avgUptime(), 0, 'f', 1));
}

void PaperDeploymentTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DeploymentEntry e;
        e.id = settings_.value("id").toInt();
        e.appName = settings_.value("appName").toString();
        e.environment = settings_.value("environment").toString();
        e.status = settings_.value("status").toString();
        e.version = settings_.value("version").toString();
        e.uptime = settings_.value("uptime").toDouble();
        e.requests = settings_.value("requests").toInt();
        e.latency = settings_.value("latency").toDouble();
        e.healthy = settings_.value("healthy").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDeploymentTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("appName", entries_[i].appName);
        settings_.setValue("environment", entries_[i].environment);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("uptime", entries_[i].uptime);
        settings_.setValue("requests", entries_[i].requests);
        settings_.setValue("latency", entries_[i].latency);
        settings_.setValue("healthy", entries_[i].healthy);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
