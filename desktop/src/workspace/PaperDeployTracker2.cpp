#include "workspace/PaperDeployTracker2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperDeployTracker2::PaperDeployTracker2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DeployTracker2")
{
    setupUI();
    loadSettings();
    if (entries_.isEmpty()) {
        QStringList services = {"API Gateway", "Auth Service", "Search Engine", "Index Worker"};
        QStringList categories = {"Backend", "Frontend", "Database", "Cache", "Queue"};
        QStringList environments = {"Production", "Staging", "Development", "Testing", "Canary"};
        QColor palette[] = {
            QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed)
        };
        for (int i = 0; i < 8; ++i) {
            DeployTracker2Entry e;
            e.id = i + 1;
            e.service = services[QRandomGenerator::global()->bounded(services.size())];
            e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
            e.environment = environments[QRandomGenerator::global()->bounded(environments.size())];
            e.uptime = 90.0 + QRandomGenerator::global()->bounded(1000) / 100.0;
            e.deployments = 1 + QRandomGenerator::global()->bounded(50);
            e.healthy = e.uptime > 95.0;
            e.color = palette[QRandomGenerator::global()->bounded(5)];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
    }
}

void PaperDeployTracker2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Backend", "Frontend", "Database", "Cache", "Queue"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search services...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    deployBtn_ = new QPushButton("Deploy");
    deployBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(deployBtn_, &QPushButton::clicked, this, &PaperDeployTracker2::onDeploy);
    toolbar->addWidget(deployBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDeployTracker2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Deploy tracker");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperDeployTracker2::addEntry(const DeployTracker2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit deployCompleted(entry.id, entry.uptime);
    update();
}

QList<DeployTracker2Entry> PaperDeployTracker2::entries() const { return entries_; }

int PaperDeployTracker2::healthyCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.healthy) ++c;
    return c;
}

qreal PaperDeployTracker2::avgUptime() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.uptime;
    return sum / entries_.size();
}

QMap<QString, int> PaperDeployTracker2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDeployTracker2::onDeploy() {
    QStringList services = {"API Gateway", "Auth Service", "Search Engine", "Index Worker"};
    QStringList categories = {"Backend", "Frontend", "Database", "Cache", "Queue"};
    QStringList environments = {"Production", "Staging", "Development", "Testing", "Canary"};
    QColor palette[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    DeployTracker2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.service = services[QRandomGenerator::global()->bounded(services.size())];
    int cIdx = categoryCombo_->currentIndex();
    e.category = cIdx == 0
        ? categories[QRandomGenerator::global()->bounded(categories.size())]
        : categories[cIdx - 1];
    e.environment = environments[QRandomGenerator::global()->bounded(environments.size())];
    e.uptime = 90.0 + QRandomGenerator::global()->bounded(1000) / 100.0;
    e.deployments = 1 + QRandomGenerator::global()->bounded(50);
    e.healthy = e.uptime > 95.0;
    e.color = palette[QRandomGenerator::global()->bounded(5)];
    addEntry(e);
}

void PaperDeployTracker2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Deploy tracker");
    update();
}

void PaperDeployTracker2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Deploy tracker");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Deployment Tracker");

    int w = width();
    int h = height();
    int statsH = static_cast<int>(h * 0.25);
    int topH = h - statsH - 70;

    drawTrackerView(p, QRect(10, 50, static_cast<int>(w * 0.6) - 10, topH));
    drawCategoryChart(p, QRect(static_cast<int>(w * 0.6) + 10, 50,
                               static_cast<int>(w * 0.4) - 20, topH));
    drawStats(p, QRect(20, h - statsH, w - 40, statsH - 10));
}

void PaperDeployTracker2::drawTrackerView(QPainter& p, const QRect& rect) {
    QString filter = inputField_->text().trimmed().toLower();
    int cIdx = categoryCombo_->currentIndex();
    QString catFilter = cIdx > 0 ? categoryCombo_->itemText(cIdx) : QString();

    QList<const DeployTracker2Entry*> visible;
    for (const auto& e : entries_) {
        if (!catFilter.isEmpty() && e.category != catFilter) continue;
        if (!filter.isEmpty() && !e.service.toLower().contains(filter)) continue;
        visible.append(&e);
    }

    int show = qMin(8, visible.size());
    if (show == 0) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 10));
        p.drawText(rect, Qt::AlignCenter, "No matching services");
        return;
    }

    int itemH = qMin(52, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = *visible[i];
        int y = rect.y() + i * (itemH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 6, 6);

        // Color accent bar on left
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Service name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 12, y + 3, rect.width() / 2 - 12, 16,
                   Qt::AlignVCenter, e.service);

        // Environment badge
        QColor badgeColor = e.color;
        p.setPen(Qt::NoPen);
        p.setBrush(badgeColor);
        int badgeW = 70;
        int badgeX = rect.x() + rect.width() - badgeW - 8;
        p.drawRoundedRect(badgeX, y + 3, badgeW, 16, 8, 8);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(badgeX, y + 3, badgeW, 16, Qt::AlignCenter, e.environment);

        // Healthy indicator
        int dotX = rect.x() + 12;
        int dotY = y + 22;
        p.setPen(Qt::NoPen);
        p.setBrush(e.healthy ? QColor(0x16, 0xa3, 0x4a) : QColor(0xdc, 0x26, 0x26));
        p.drawEllipse(dotX, dotY + 2, 8, 8);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(dotX + 12, dotY, 60, 12, Qt::AlignVCenter,
                   e.healthy ? "Healthy" : "Unhealthy");

        // Uptime bar
        int barX = dotX + 80;
        int barW = rect.width() / 2 - 100;
        int barY = dotY + 3;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barW, 6, 3, 3);

        QColor uptimeColor = e.uptime > 99.0
            ? QColor(0x16, 0xa3, 0x4a)
            : (e.uptime > 95.0 ? QColor(0xd9, 0x77, 0x06) : QColor(0xdc, 0x26, 0x26));
        qreal ratio = qMin(e.uptime / 100.0, 1.0);
        int fillW = static_cast<int>(ratio * barW);
        p.setBrush(uptimeColor);
        p.drawRoundedRect(barX, barY, fillW, 6, 3, 3);

        // Uptime text
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(barX + barW + 4, dotY, 50, 12, Qt::AlignVCenter,
                   QString::number(e.uptime, 'f', 1) + "%");

        // Deployment count on the right
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, dotY,
                   rect.width() / 2 - 80, 12,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.deployments) + " deploys");
    }
}

void PaperDeployTracker2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Environment Distribution");

    QStringList environments = {"Production", "Staging", "Development", "Testing", "Canary"};
    QString labels[] = {"Prod", "Stage", "Dev", "Test", "Canary"};
    QColor colors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    QMap<QString, int> envCounts;
    for (const auto& e : entries_) envCounts[e.environment]++;

    int maxVal = 1;
    for (const auto& v : envCounts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 40) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 24 + i * (barH + 4);
        int count = envCounts.contains(environments[i]) ? envCounts[environments[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 4, 50, barH, Qt::AlignRight | Qt::AlignVCenter,
                   labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 56, y, barW, barH - 4, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 60 + barW, y + barH - 4, QString::number(count));
    }
}

void PaperDeployTracker2::drawStats(QPainter& p, const QRect& rect) {
    int totalDeploys = 0;
    for (const auto& e : entries_) totalDeploys += e.deployments;

    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Services",  QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Healthy Count",   QString::number(healthyCount()),  QColor(0x16, 0xa3, 0x4a)},
        {"Avg Uptime",      QString::number(avgUptime(), 'f', 1) + "%",
         QColor(0xd9, 0x77, 0x06)},
        {"Total Deploys",   QString::number(totalDeploys),    QColor(0x7c, 0x3a, 0xed)}
    };

    int boxW = (rect.width() - 30) / 4;
    int boxH = qMin(56, rect.height() - 4);

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        // Top color accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(x, y, boxW, 3, 2, 2);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, y + 8, boxW - 20, 24, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + 32, boxW - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperDeployTracker2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Deploy tracker");
        return;
    }
    infoLabel_->setText(
        QString("%1 services | %2 healthy | avg %3% uptime")
            .arg(entries_.size())
            .arg(healthyCount())
            .arg(avgUptime(), 0, 'f', 1));
}

void PaperDeployTracker2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DeployTracker2Entry e;
        e.id = settings_.value("id").toInt();
        e.service = settings_.value("service").toString();
        e.category = settings_.value("category").toString();
        e.environment = settings_.value("environment").toString();
        e.uptime = settings_.value("uptime").toDouble();
        e.deployments = settings_.value("deployments").toInt();
        e.healthy = settings_.value("healthy").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDeployTracker2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("service", entries_[i].service);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("environment", entries_[i].environment);
        settings_.setValue("uptime", entries_[i].uptime);
        settings_.setValue("deployments", entries_[i].deployments);
        settings_.setValue("healthy", entries_[i].healthy);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
