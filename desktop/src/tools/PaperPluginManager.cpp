#include "tools/PaperPluginManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperPluginManager::PaperPluginManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PluginManager")
{
    setupUI();
    loadSettings();
}

void PaperPluginManager::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    loadBtn_ = new QPushButton("Load");
    loadBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(loadBtn_, &QPushButton::clicked, this, &PaperPluginManager::onLoad);
    toolbar->addWidget(loadBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Analysis", "Visualization", "Import", "Export"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPluginManager::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter plugin name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Manage plugins");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperPluginManager::addEntry(const PluginEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pluginLoaded(entry.id, entry.name);
    update();
}

QList<PluginEntry> PaperPluginManager::entries() const { return entries_; }

int PaperPluginManager::enabledCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.enabled) c++;
    return c;
}

qreal PaperPluginManager::avgRating() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.rating;
    return sum / entries_.size();
}

QMap<QString, int> PaperPluginManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPluginManager::onLoad() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"analysis", "visualization", "import", "export"};
    QStringList authors = {"dev-A", "dev-B", "community-C", "official"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        PluginEntry e;
        e.id = entries_.size() + 1;
        e.name = text.left(8) + " plug" + QString::number(i);
        e.version = QString::number(1 + QRandomGenerator::global()->bounded(3)) + "."
                   + QString::number(QRandomGenerator::global()->bounded(10));
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.author = authors[QRandomGenerator::global()->bounded(authors.size())];
        e.downloads = QRandomGenerator::global()->bounded(10000);
        e.rating = 1 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.enabled = QRandomGenerator::global()->bounded(4) != 0;
        e.updated = QRandomGenerator::global()->bounded(3) == 0;
        e.color = e.enabled ? (e.updated ? QColor(245,158,11) : QColor(16,185,129)) : QColor(156,163,175);
        addEntry(e);
    }
    inputField_->clear();
}

void PaperPluginManager::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage plugins");
    update();
}

void PaperPluginManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage plugins");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Plugin Manager");
    int w = width(), h = height();
    drawPluginList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperPluginManager::drawPluginList(QPainter& p, const QRect& rect) {
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
                   e.name.left(14) + " v" + e.version + (e.updated ? " [U]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.author + " | " + e.category + " | " + QString::number(e.downloads) + " dl");
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.rating, 'f', 1) + "/5.0");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.enabled ? "enabled" : "disabled");
    }
}

void PaperPluginManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"analysis", "visualization", "import", "export"};
    QString labels[] = {"Analysis", "Visual", "Import", "Export"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 55, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperPluginManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Plugins", QString::number(entries_.size()), QColor(59,130,246)},
        {"Enabled", QString::number(enabledCount()), QColor(16,185,129)},
        {"Avg Rating", QString::number(avgRating(), 'f', 1) + "/5", QColor(245,158,11)},
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

void PaperPluginManager::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage plugins"); return; }
    infoLabel_->setText(QString("%1 plugins | %2 enabled | %3/5 avg")
        .arg(entries_.size()).arg(enabledCount()).arg(avgRating(), 0, 'f', 1));
}

void PaperPluginManager::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PluginEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.version = settings_.value("version").toString();
        e.category = settings_.value("category").toString();
        e.author = settings_.value("author").toString();
        e.downloads = settings_.value("downloads").toInt();
        e.rating = settings_.value("rating").toDouble();
        e.enabled = settings_.value("enabled").toBool();
        e.updated = settings_.value("updated").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPluginManager::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("author", entries_[i].author);
        settings_.setValue("downloads", entries_[i].downloads);
        settings_.setValue("rating", entries_[i].rating);
        settings_.setValue("enabled", entries_[i].enabled);
        settings_.setValue("updated", entries_[i].updated);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
