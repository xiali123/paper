#include "tools/PaperDeployTracker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDeployTracker::PaperDeployTracker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DeployTracker")
{
    setupUI();
    loadSettings();
}

void PaperDeployTracker::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    deployBtn_ = new QPushButton("Deploy");
    deployBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(deployBtn_, &QPushButton::clicked, this, &PaperDeployTracker::onDeploy);
    toolbar->addWidget(deployBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Frontend", "Backend", "API", "Database", "Infrastructure"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Version tag...");
    toolbar->addWidget(inputField_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDeployTracker::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track paper deployments");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 460);
}

void PaperDeployTracker::addEntry(const DeployEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit deployComplete(entry.id, entry.uptime);
    update();
}

QList<DeployEntry> PaperDeployTracker::entries() const { return entries_; }

int PaperDeployTracker::stableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.stable) c++;
    return c;
}

qreal PaperDeployTracker::avgUptime() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.uptime;
    return sum / entries_.size();
}

QMap<QString, int> PaperDeployTracker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDeployTracker::onDeploy() {
    QStringList categories = {"Frontend", "Backend", "API", "Database", "Infrastructure"};
    QStringList environments = {"Production", "Staging", "Development", "Testing"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int count = QRandomGenerator::global()->bounded(4, 9);
    QString version = inputField_->text().isEmpty()
        ? QString("v%1.%2.%3").arg(QRandomGenerator::global()->bounded(1, 5))
                                .arg(QRandomGenerator::global()->bounded(0, 20))
                                .arg(QRandomGenerator::global()->bounded(0, 50))
        : inputField_->text();

    for (int i = 0; i < count; ++i) {
        DeployEntry e;
        e.id = entries_.size() + 1;
        e.version = version;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.environment = environments[QRandomGenerator::global()->bounded(environments.size())];
        e.uptime = QRandomGenerator::global()->bounded(8500, 10000) / 100.0;
        e.deploys = QRandomGenerator::global()->bounded(1, 20);
        e.stable = e.uptime >= 95.0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
}

void PaperDeployTracker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track paper deployments");
    update();
}

void PaperDeployTracker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Track paper deployments");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Deploy Tracker");

    int w = width(), h = height();
    drawDeployTimeline(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDeployTracker::drawDeployTimeline(QPainter& p, const QRect& rect) {
    int maxShow = 12;
    int itemH = qMin(34, (rect.height() - 10) / maxShow);
    int start = qMax(0, entries_.size() - maxShow);
    int shown = 0;

    for (int i = entries_.size() - 1; i >= start && shown < maxShow; --i) {
        const auto& e = entries_[i];
        int y = rect.y() + shown * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.stable ? QColor(236, 253, 245) : QColor(254, 242, 242));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(rect.x() + 6, y + itemH / 2 - 4, 8, 8);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 20, y + 2, rect.width() - 80, 16, Qt::AlignVCenter,
                   e.version + " - " + e.category);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 20, y + 18, rect.width() - 80, 14, Qt::AlignVCenter,
                   e.environment + " | " + QString::number(e.deploys) + " deploys");

        p.setPen(e.stable ? QColor(22, 163, 74) : QColor(220, 38, 38));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 56, y + 4, 52, itemH - 8, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.uptime, 'f', 1) + "%");
        shown++;
    }
}

void PaperDeployTracker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"Frontend", "Backend", "API", "Database", "Infrastructure"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 130));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 80, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperDeployTracker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Deploys", QString::number(entries_.size()), QColor(59,130,246)},
        {"Stable", QString::number(stableCount()), QColor(22,163,74)},
        {"Avg Uptime", QString::number(avgUptime(), 'f', 1) + "%", QColor(217,119,6)},
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

void PaperDeployTracker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Track paper deployments"); return; }
    infoLabel_->setText(QString("%1 deploys | %2 stable | %3% avg uptime")
        .arg(entries_.size()).arg(stableCount())
        .arg(QString::number(avgUptime(), 'f', 1)));
}

void PaperDeployTracker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DeployEntry e;
        e.id = settings_.value("id").toInt();
        e.version = settings_.value("version").toString();
        e.category = settings_.value("category").toString();
        e.environment = settings_.value("environment").toString();
        e.uptime = settings_.value("uptime").toReal();
        e.deploys = settings_.value("deploys").toInt();
        e.stable = settings_.value("stable").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDeployTracker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("environment", entries_[i].environment);
        settings_.setValue("uptime", entries_[i].uptime);
        settings_.setValue("deploys", entries_[i].deploys);
        settings_.setValue("stable", entries_[i].stable);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
