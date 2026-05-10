#include "tools/PaperUpdateChecker.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperUpdateChecker::PaperUpdateChecker(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "UpdateChecker")
{
    setupUI();
    loadSettings();
}

void PaperUpdateChecker::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    checkBtn_ = new QPushButton("Check");
    checkBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(checkBtn_, &QPushButton::clicked, this, &PaperUpdateChecker::onCheck);
    toolbar->addWidget(checkBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Core", "Plugin", "Theme", "Library"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperUpdateChecker::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter component name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Check for updates");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperUpdateChecker::addEntry(const UpdateEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit updateFound(entry.id, entry.version);
    update();
}

QList<UpdateEntry> PaperUpdateChecker::entries() const { return entries_; }

int PaperUpdateChecker::criticalCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.critical) c++;
    return c;
}

int PaperUpdateChecker::installedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.installed) c++;
    return c;
}

QMap<QString, int> PaperUpdateChecker::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperUpdateChecker::onCheck() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"core", "plugin", "theme", "library"};
    QStringList severities = {"critical", "high", "medium", "low"};
    QStringList components = {"engine", "parser", "renderer", "indexer", "crawler"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        UpdateEntry e;
        e.id = entries_.size() + 1;
        e.version = QString::number(1 + QRandomGenerator::global()->bounded(3)) + "."
                   + QString::number(QRandomGenerator::global()->bounded(10)) + "."
                   + QString::number(QRandomGenerator::global()->bounded(20));
        e.component = components[QRandomGenerator::global()->bounded(components.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.severity = severities[QRandomGenerator::global()->bounded(severities.size())];
        e.changelog = text.left(8) + " fix" + QString::number(i);
        e.installed = QRandomGenerator::global()->bounded(3) == 0;
        e.critical = e.severity == "critical";
        e.color = e.critical ? QColor(239,68,68) : (e.installed ? QColor(16,185,129) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperUpdateChecker::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Check for updates");
    update();
}

void PaperUpdateChecker::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Check for updates");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Update Checker");
    int w = width(), h = height();
    drawUpdateList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperUpdateChecker::drawUpdateList(QPainter& p, const QRect& rect) {
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
                   e.component + " v" + e.version + (e.critical ? " [!]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.severity + " | " + e.category + " | " + e.changelog.left(10));
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.critical ? "critical" : e.severity);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.installed ? "installed" : "pending");
    }
}

void PaperUpdateChecker::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"core", "plugin", "theme", "library"};
    QString labels[] = {"Core", "Plugin", "Theme", "Library"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
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

void PaperUpdateChecker::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Updates", QString::number(entries_.size()), QColor(59,130,246)},
        {"Critical", QString::number(criticalCount()), QColor(239,68,68)},
        {"Installed", QString::number(installedCount()), QColor(16,185,129)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperUpdateChecker::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Check for updates"); return; }
    infoLabel_->setText(QString("%1 updates | %2 critical | %3 installed")
        .arg(entries_.size()).arg(criticalCount()).arg(installedCount()));
}

void PaperUpdateChecker::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        UpdateEntry e;
        e.id = settings_.value("id").toInt();
        e.version = settings_.value("version").toString();
        e.component = settings_.value("component").toString();
        e.category = settings_.value("category").toString();
        e.severity = settings_.value("severity").toString();
        e.changelog = settings_.value("changelog").toString();
        e.installed = settings_.value("installed").toBool();
        e.critical = settings_.value("critical").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperUpdateChecker::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("component", entries_[i].component);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("severity", entries_[i].severity);
        settings_.setValue("changelog", entries_[i].changelog);
        settings_.setValue("installed", entries_[i].installed);
        settings_.setValue("critical", entries_[i].critical);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
