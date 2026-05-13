#include "tools/PaperVersionTree2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperVersionTree2::PaperVersionTree2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "VersionTree2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Release", "Beta", "Alpha", "Dev", "Patch"};
        QStringList branches = {"main", "develop", "feature", "hotfix", "release"};
        QColor catColors[] = {
            QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed)
        };
        for (int i = 0; i < 8; ++i) {
            VersionTree2Entry e;
            e.id = i + 1;
            e.version = QString::number(1 + QRandomGenerator::global()->bounded(3)) + "."
                       + QString::number(QRandomGenerator::global()->bounded(10)) + "."
                       + QString::number(QRandomGenerator::global()->bounded(20));
            int ci = QRandomGenerator::global()->bounded(categories.size());
            e.category = categories[ci];
            e.branch = branches[QRandomGenerator::global()->bounded(branches.size())];
            e.size = 0.5 + QRandomGenerator::global()->bounded(50) / 10.0;
            e.commits = 1 + QRandomGenerator::global()->bounded(40);
            e.stable = (e.category == "Release");
            e.color = catColors[ci];
            entries_.append(e);
        }
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperVersionTree2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Release", "Beta", "Alpha", "Dev", "Patch"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Version (e.g. 2.1.3)...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperVersionTree2::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperVersionTree2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Version Tree 2");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperVersionTree2::addEntry(const VersionTree2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit versionSelected(entry.id, entry.size);
    update();
}

QList<VersionTree2Entry> PaperVersionTree2::entries() const {
    return entries_;
}

int PaperVersionTree2::stableCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.stable) ++c;
    return c;
}

qreal PaperVersionTree2::avgSize() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.size;
    return total / entries_.size();
}

QMap<QString, int> PaperVersionTree2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperVersionTree2::onRender() {
    QString version = inputField_->text().trimmed();
    if (version.isEmpty()) return;

    QStringList categories = {"Release", "Beta", "Alpha", "Dev", "Patch"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };
    int ci = categoryCombo_->currentIndex();
    if (ci == 0) ci = 1 + QRandomGenerator::global()->bounded(categories.size());
    else ci = ci;

    VersionTree2Entry e;
    e.id = entries_.size() + 1;
    e.version = version;
    e.category = categories[ci - 1 < 0 ? 0 : ci - 1 < categories.size() ? ci - 1 : 0];
    QStringList branches = {"main", "develop", "feature", "hotfix", "release"};
    e.branch = branches[QRandomGenerator::global()->bounded(branches.size())];
    e.size = 0.5 + QRandomGenerator::global()->bounded(50) / 10.0;
    e.commits = 1 + QRandomGenerator::global()->bounded(40);
    e.stable = (e.category == "Release");
    e.color = catColors[categories.indexOf(e.category) >= 0 ? categories.indexOf(e.category) : 0];
    addEntry(e);
    inputField_->clear();
}

void PaperVersionTree2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Version Tree 2");
    update();
}

void PaperVersionTree2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No version entries");
        return;
    }

    int w = width(), h = height();

    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 28, "Version Tree 2");

    // Top-left: tree view, Top-right: category chart, Bottom: stats
    int topH = (h - 70) * 3 / 5;
    int halfW = w / 2;
    drawTreeView(p, QRect(20, 45, halfW - 30, topH));
    drawCategoryChart(p, QRect(halfW, 45, halfW - 30, topH));
    drawStats(p, QRect(20, 45 + topH + 15, w - 40, h - topH - 85));
}

void PaperVersionTree2::drawTreeView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() - 2, "Tree View");

    int show = qMin(10, entries_.size());
    if (show == 0) return;

    int nodeR = 10;
    int levelH = qMin(42, (rect.height() - 20) / qMax(show, 1));
    int startX = rect.x() + 30;

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 18 + i * levelH;

        // Indent by branch level
        int indent = 0;
        if (e.branch == "develop") indent = 20;
        else if (e.branch == "feature") indent = 40;
        else if (e.branch == "hotfix") indent = 60;
        else if (e.branch == "release") indent = 10;
        int nx = startX + indent;

        // Branch line from parent
        if (i > 0) {
            int prevIndent = 0;
            if (entries_[i - 1].branch == "develop") prevIndent = 20;
            else if (entries_[i - 1].branch == "feature") prevIndent = 40;
            else if (entries_[i - 1].branch == "hotfix") prevIndent = 60;
            else if (entries_[i - 1].branch == "release") prevIndent = 10;
            int px = startX + prevIndent;
            int py = rect.y() + 18 + (i - 1) * levelH + nodeR;

            p.setPen(QPen(e.color.lighter(150), 1.5));
            if (nx == px) {
                // Straight line down
                p.drawLine(px, py, nx, y);
            } else {
                // Angled branch line
                p.drawLine(px, py, px, py + levelH / 3);
                p.drawLine(px, py + levelH / 3, nx, y - levelH / 3);
                p.drawLine(nx, y - levelH / 3, nx, y);
            }
        }

        // Node circle
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(nx - nodeR, y, nodeR * 2, nodeR * 2);

        // Stable indicator ring
        if (e.stable) {
            p.setPen(QPen(QColor(22, 163, 74), 2));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(nx - nodeR - 2, y - 2, (nodeR + 2) * 2, (nodeR + 2) * 2);
        }

        // Version label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(nx + nodeR + 8, y + 4, rect.width() - indent - nodeR * 2 - 20, 16,
                   Qt::AlignVCenter, e.version + "  [" + e.category + "]");

        // Commit count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(nx + nodeR + 8, y + 18, rect.width() - indent - nodeR * 2 - 20, 14,
                   Qt::AlignVCenter,
                   e.branch + " | " + QString::number(e.commits) + " commits | "
                   + QString::number(e.size, 'f', 1) + " MB");
    }
}

void PaperVersionTree2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.x(), rect.y() - 2, "Categories");

    auto counts = categoryCounts();
    QStringList categories = {"Release", "Beta", "Alpha", "Dev", "Patch"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int total = 0;
    for (int i = 0; i < categories.size(); ++i)
        total += counts.contains(categories[i]) ? counts[categories[i]] : 0;

    if (total == 0) return;

    // Donut chart
    int side = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 24 + side / 2;
    int outerR = side / 2 - 4;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 0;
    for (int i = 0; i < categories.size(); ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal span = 360.0 * count / total;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    // Inner circle for donut hole
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(cx - innerR, cy - 10, innerR * 2, 20,
               Qt::AlignCenter, QString::number(total));
    p.setFont(QFont("Arial", 8));
    p.setPen(QColor(100, 116, 139));
    p.drawText(cx - innerR, cy + 8, innerR * 2, 16,
               Qt::AlignCenter, "entries");

    // Legend below chart
    int legendY = cy + outerR + 10;
    int legendW = rect.width() / 3;
    for (int i = 0; i < categories.size(); ++i) {
        int col = i % 3;
        int row = i / 3;
        int lx = rect.x() + col * legendW;
        int ly = legendY + row * 18;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawRoundedRect(lx, ly, 10, 10, 2, 2);

        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(QColor(71, 85, 105));
        p.setFont(QFont("Arial", 8));
        p.drawText(lx + 14, ly + 10, categories[i] + " (" + QString::number(count) + ")");
    }
}

void PaperVersionTree2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries",  QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Stable",         QString::number(stableCount()),  QColor(0x16, 0xa3, 0x4a)},
        {"Avg Size",       QString::number(avgSize(), 'f', 1) + " MB", QColor(0xd9, 0x77, 0x06)},
        {"Total Commits",  QString::number([&]{
            int c = 0; for (const auto& e : entries_) c += e.commits; return c; }()),
                                             QColor(0x7c, 0x3a, 0xed)}
    };

    int cols = 2;
    int rows = 2;
    int gapX = 10, gapY = 8;
    int boxW = (rect.width() - gapX * (cols - 1)) / cols;
    int boxH = (rect.height() - gapY * (rows - 1)) / rows;

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int bx = rect.x() + col * (boxW + gapX);
        int by = rect.y() + row * (boxH + gapY);

        // Background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 6, 6);

        // Color accent bar
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, by, 4, boxH, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(bx + 12, by + 4, boxW - 20, boxH / 2, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 12, by + boxH / 2, boxW - 20, boxH / 2 - 4, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperVersionTree2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Version Tree 2");
        return;
    }
    int totalCommits = 0;
    for (const auto& e : entries_) totalCommits += e.commits;
    infoLabel_->setText(
        QString("%1 entries | %2 stable | Avg %3 MB | %4 commits")
            .arg(entries_.size())
            .arg(stableCount())
            .arg(QString::number(avgSize(), 'f', 1))
            .arg(totalCommits));
}

void PaperVersionTree2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        VersionTree2Entry e;
        e.id = settings_.value("id").toInt();
        e.version = settings_.value("version").toString();
        e.category = settings_.value("category").toString();
        e.branch = settings_.value("branch").toString();
        e.size = settings_.value("size").toReal();
        e.commits = settings_.value("commits").toInt();
        e.stable = settings_.value("stable").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperVersionTree2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("version", entries_[i].version);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("branch", entries_[i].branch);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("commits", entries_[i].commits);
        settings_.setValue("stable", entries_[i].stable);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
