#include "tools/PaperPluginInstaller.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperPluginInstaller::PaperPluginInstaller(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PluginInstaller")
{
    setupUI();
    loadSettings();
}

void PaperPluginInstaller::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    installBtn_ = new QPushButton("Install");
    installBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(installBtn_, &QPushButton::clicked, this, &PaperPluginInstaller::onInstall);
    toolbar->addWidget(installBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Analysis", "Visualization", "Import", "Export"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPluginInstaller::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter plugin name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Install plugins");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperPluginInstaller::addEntry(const PluginEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pluginInstalled(entry.id, entry.status);
    update();
}

QList<PluginEntry> PaperPluginInstaller::entries() const { return entries_; }

int PaperPluginInstaller::enabledCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.enabled) c++;
    return c;
}

qreal PaperPluginInstaller::avgRating() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.rating;
    return sum / entries_.size();
}

QMap<QString, int> PaperPluginInstaller::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPluginInstaller::onInstall() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"analysis", "visualization", "import", "export"};
    QStringList authors = {"research-lab", "community", "official", "third-party"};
    QStringList statuses = {"installed", "available", "update", "disabled"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        PluginEntry e;
        e.id = entries_.size() + 1;
        e.pluginName = text.left(10).toLower() + "-plugin-" + QString::number(i);
        e.version = "1." + QString::number(QRandomGenerator::global()->bounded(10)) + "." + QString::number(QRandomGenerator::global()->bounded(20));
        e.author = authors[QRandomGenerator::global()->bounded(authors.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.downloads = 10 + QRandomGenerator::global()->bounded(10000);
        e.rating = 1 + QRandomGenerator::global()->bounded(40) / 10.0;
        e.status = statuses[QRandomGenerator::global()->bounded(statuses.size())];
        e.installedDate = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(10));
        e.enabled = e.status == "installed";
        e.color = e.enabled ? QColor(16,185,129) : (e.status == "available" ? QColor(59,130,246) : QColor(156,163,175));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperPluginInstaller::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Install plugins");
    update();
}

void PaperPluginInstaller::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Install plugins");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Plugin Installer");
    int w = width(), h = height();
    drawPluginList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperPluginInstaller::drawPluginList(QPainter& p, const QRect& rect) {
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
                   e.pluginName.left(16) + (e.enabled ? " [ON]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   "v" + e.version + " | " + e.author + " | " + e.status);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.rating, 'f', 1) + "/5.0");
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.downloads) + " dl | " + e.installedDate);
    }
}

void PaperPluginInstaller::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"analysis", "visualization", "import", "export"};
    QString labels[] = {"Analysis", "Visual", "Import", "Export"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};
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

void PaperPluginInstaller::drawStats(QPainter& p, const QRect& rect) {
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

void PaperPluginInstaller::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Install plugins"); return; }
    infoLabel_->setText(QString("%1 plugins | %2 enabled | %3/5 avg")
        .arg(entries_.size()).arg(enabledCount()).arg(avgRating(), 0, 'f', 1));
}

void PaperPluginInstaller::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        PluginEntry e;
        e.id = settings_.value("id").toInt();
        e.pluginName = settings_.value("pluginName").toString();
        e.version = settings_.value("version").toString();
        e.author = settings_.value("author").toString();
        e.category = settings_.value("category").toString();
        e.downloads = settings_.value("downloads").toInt();
        e.rating = settings_.value("rating").toDouble();
        e.status = settings_.value("status").toString();
        e.installedDate = settings_.value("installedDate").toString();
        e.enabled = settings_.value("enabled").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperPluginInstaller::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("pluginName", entries_[i].pluginName);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("author", entries_[i].author);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("downloads", entries_[i].downloads);
        settings_.setValue("rating", entries_[i].rating);
        settings_.setValue("status", entries_[i].status);
        settings_.setValue("installedDate", entries_[i].installedDate);
        settings_.setValue("enabled", entries_[i].enabled);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
