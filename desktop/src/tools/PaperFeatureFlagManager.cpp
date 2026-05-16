#include "tools/PaperFeatureFlagManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperFeatureFlagManager::PaperFeatureFlagManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "FeatureFlagManager")
{
    setupUI();
    loadSettings();
}

void PaperFeatureFlagManager::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    toggleBtn_ = new QPushButton("Toggle");
    toggleBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(toggleBtn_, &QPushButton::clicked, this, &PaperFeatureFlagManager::onToggle);
    toolbar->addWidget(toggleBtn_);
    toolbar->addWidget(new QLabel("Env:"));
    envCombo_ = new QComboBox();
    envCombo_->addItems({"All", "Production", "Staging", "Development"});
    toolbar->addWidget(envCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperFeatureFlagManager::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter feature flag name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Manage feature flags");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperFeatureFlagManager::addEntry(const FeatureFlagEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit flagToggled(entry.id, entry.enabled);
    update();
}

QList<FeatureFlagEntry> PaperFeatureFlagManager::entries() const { return entries_; }

int PaperFeatureFlagManager::enabledCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.enabled) c++;
    return c;
}

int PaperFeatureFlagManager::stableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.stable) c++;
    return c;
}

QMap<QString, int> PaperFeatureFlagManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperFeatureFlagManager::onToggle() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList environments = {"production", "staging", "development"};
    QStringList rollouts = {"gradual", "instant", "scheduled", "percentage"};
    QStringList descriptions = {"New search algorithm", "Enhanced UI layout", "Improved caching", "Beta ML features"};
    QStringList categories = {"search", "ui", "performance", "ml", "security"};
    int eIdx = envCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        FeatureFlagEntry e;
        e.id = entries_.size() + 1;
        e.flagName = text.left(10).toLower() + "_flag_" + QString::number(i);
        e.environment = eIdx == 0 ? environments[QRandomGenerator::global()->bounded(environments.size())] : environments[eIdx - 1];
        e.enabled = QRandomGenerator::global()->bounded(2) == 0;
        e.description = descriptions[QRandomGenerator::global()->bounded(descriptions.size())];
        e.rollout = rollouts[QRandomGenerator::global()->bounded(rollouts.size())];
        e.userPercentage = e.enabled ? (10 + QRandomGenerator::global()->bounded(91)) : 0;
        e.createdDate = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(10));
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.stable = e.enabled && e.userPercentage >= 80;
        e.color = e.enabled ? (e.stable ? QColor(16,185,129) : QColor(59,130,246)) : QColor(156,163,175);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperFeatureFlagManager::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage feature flags");
    update();
}

void PaperFeatureFlagManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage feature flags");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Feature Flag Manager");
    int w = width(), h = height();
    drawFlagList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperFeatureFlagManager::drawFlagList(QPainter& p, const QRect& rect) {
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
                   e.flagName.left(18) + (e.enabled ? " [ON]" : " [OFF]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.environment + " | " + e.rollout + " | " + e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.userPercentage) + "% users");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.createdDate);
    }
}

void PaperFeatureFlagManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"search", "ui", "performance", "ml", "security"};
    QString labels[] = {"Search", "UI", "Perf", "ML", "Security"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246), QColor(239,68,68)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(20, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperFeatureFlagManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Flags", QString::number(entries_.size()), QColor(59,130,246)},
        {"Enabled", QString::number(enabledCount()), QColor(16,185,129)},
        {"Stable", QString::number(stableCount()), QColor(139,92,246)},
        {"Categories", QString::number(categoryCounts().size()), QColor(245,158,11)}
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

void PaperFeatureFlagManager::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage feature flags"); return; }
    infoLabel_->setText(QString("%1 flags | %2 on | %3 stable")
        .arg(entries_.size()).arg(enabledCount()).arg(stableCount()));
}

void PaperFeatureFlagManager::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FeatureFlagEntry e;
        e.id = settings_.value("id").toInt();
        e.flagName = settings_.value("flagName").toString();
        e.environment = settings_.value("environment").toString();
        e.enabled = settings_.value("enabled").toBool();
        e.description = settings_.value("description").toString();
        e.rollout = settings_.value("rollout").toString();
        e.userPercentage = settings_.value("userPercentage").toInt();
        e.createdDate = settings_.value("createdDate").toString();
        e.category = settings_.value("category").toString();
        e.stable = settings_.value("stable").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperFeatureFlagManager::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("flagName", entries_[i].flagName);
        settings_.setValue("environment", entries_[i].environment);
        settings_.setValue("enabled", entries_[i].enabled);
        settings_.setValue("description", entries_[i].description);
        settings_.setValue("rollout", entries_[i].rollout);
        settings_.setValue("userPercentage", entries_[i].userPercentage);
        settings_.setValue("createdDate", entries_[i].createdDate);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("stable", entries_[i].stable);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
