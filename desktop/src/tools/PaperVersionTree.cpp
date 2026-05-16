#include "tools/PaperVersionTree.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperVersionTree::PaperVersionTree(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "VersionTree")
{
    setupUI();
    loadSettings();
}

void PaperVersionTree::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperVersionTree::onAdd);
    toolbar->addWidget(addBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Release", "Beta", "Alpha", "Dev"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVersionTree::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter version tag...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("View version tree");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperVersionTree::addEntry(const VersionEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit versionAdded(entry.id, entry.version);
    update();
}

QList<VersionEntry> PaperVersionTree::entries() const { return entries_; }

int PaperVersionTree::stableCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.stable) c++;
    return c;
}

QMap<QString, int> PaperVersionTree::branchCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.branch]++;
    return counts;
}

QMap<QString, int> PaperVersionTree::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperVersionTree::onAdd() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"release", "beta", "alpha", "dev"};
    QStringList branches = {"main", "develop", "feature", "hotfix"};
    QStringList tags = {"v1.0", "v1.1", "v2.0", "v2.1", "v3.0"};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        VersionEntry e;
        e.id = entries_.size() + 1;
        e.version = QString::number(1 + QRandomGenerator::global()->bounded(3)) + "."
                   + QString::number(QRandomGenerator::global()->bounded(10)) + "."
                   + QString::number(QRandomGenerator::global()->bounded(20));
        e.branch = branches[QRandomGenerator::global()->bounded(branches.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.tag = tags[QRandomGenerator::global()->bounded(tags.size())];
        e.date = "2026-05-" + QString::number(1 + QRandomGenerator::global()->bounded(28));
        e.changes = 1 + QRandomGenerator::global()->bounded(30);
        e.stable = e.category == "release";
        e.latest = QRandomGenerator::global()->bounded(5) == 0;
        e.color = e.latest ? QColor(245,158,11) : (e.stable ? QColor(16,185,129) : QColor(59,130,246));
        addEntry(e);
    }
    inputField_->clear();
}

void PaperVersionTree::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("View version tree");
    update();
}

void PaperVersionTree::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "View version tree");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Version Tree");
    int w = width(), h = height();
    drawVersionList(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawBranchChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperVersionTree::drawVersionList(QPainter& p, const QRect& rect) {
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
                   e.version + (e.latest ? " [L]" : "") + (e.stable ? " [S]" : ""));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.branch + " | " + e.tag + " | " + e.date);
        p.drawText(rect.x() + rect.width() / 2, y + 4, rect.width() / 2 - 10, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   e.category);
        p.drawText(rect.x() + rect.width() / 2, y + 20, rect.width() / 2 - 10, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.changes) + " changes");
    }
}

void PaperVersionTree::drawBranchChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Branches");
    auto counts = branchCounts();
    QStringList branches = {"main", "develop", "feature", "hotfix"};
    QString labels[] = {"Main", "Develop", "Feature", "Hotfix"};
    QColor colors[] = {QColor(16,185,129), QColor(59,130,246), QColor(245,158,11), QColor(239,68,68)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(branches[i]) ? counts[branches[i]] : 0;
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

void PaperVersionTree::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Versions", QString::number(entries_.size()), QColor(59,130,246)},
        {"Stable", QString::number(stableCount()), QColor(16,185,129)},
        {"Branches", QString::number(branchCounts().size()), QColor(245,158,11)},
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

void PaperVersionTree::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("View version tree"); return; }
    infoLabel_->setText(QString("%1 versions | %2 stable | %3 branches")
        .arg(entries_.size()).arg(stableCount()).arg(branchCounts().size()));
}

void PaperVersionTree::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        VersionEntry e;
        e.id = settings_.value("id").toInt();
        e.version = settings_.value("version").toString();
        e.branch = settings_.value("branch").toString();
        e.category = settings_.value("category").toString();
        e.tag = settings_.value("tag").toString();
        e.date = settings_.value("date").toString();
        e.changes = settings_.value("changes").toInt();
        e.stable = settings_.value("stable").toBool();
        e.latest = settings_.value("latest").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperVersionTree::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("branch", entries_[i].branch);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("tag", entries_[i].tag);
        settings_.setValue("date", entries_[i].date);
        settings_.setValue("changes", entries_[i].changes);
        settings_.setValue("stable", entries_[i].stable);
        settings_.setValue("latest", entries_[i].latest);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
