#include "tools/PaperIndexManager.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperIndexManager::PaperIndexManager(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "IndexManager")
{
    setupUI();
    loadSettings();
}

void PaperIndexManager::addEntry(const IndexEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<IndexEntry> PaperIndexManager::entries() const { return entries_; }

int PaperIndexManager::activeCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.active) c++;
    return c;
}

qreal PaperIndexManager::totalSize() const {
    qreal s = 0;
    for (const auto& e : entries_) s += e.size;
    return s;
}

QMap<QString, int> PaperIndexManager::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperIndexManager::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    rebuildBtn_ = new QPushButton("Rebuild");
    rebuildBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(rebuildBtn_, &QPushButton::clicked, this, &PaperIndexManager::onRebuild);
    toolbar->addWidget(rebuildBtn_);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Author", "Keyword", "Reference", "Institution"});
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search index...");
    toolbar->addWidget(inputField_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperIndexManager::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Manage paper indexes");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 460);
}

void PaperIndexManager::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Manage paper indexes");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Index Manager");

    int w = width(), h = height();
    drawIndexView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperIndexManager::drawIndexView(QPainter& p, const QRect& rect) {
    int catIdx = categoryCombo_->currentIndex();
    int show = 0;
    int maxShow = 10;
    int itemH = qMin(36, (rect.height() - 10) / maxShow);

    for (int i = entries_.size() - 1; i >= 0 && show < maxShow; --i) {
        const auto& e = entries_[i];

        if (catIdx == 1 && e.category != "Author") continue;
        if (catIdx == 2 && e.category != "Keyword") continue;
        if (catIdx == 3 && e.category != "Reference") continue;
        if (catIdx == 4 && e.category != "Institution") continue;

        int y = rect.y() + show * (itemH + 3);

        p.setPen(Qt::NoPen);
        p.setBrush(e.active ? e.color.lighter(190) : Qt::white);
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 4, 4);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(rect.x() + 6, y + itemH / 2 - 4, 8, 8);

        if (!e.active) {
            p.setBrush(QColor(203, 213, 225));
            p.drawEllipse(rect.x() + 6, y + itemH / 2 - 4, 8, 8);
        }

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 20, y + 2, rect.width() - 50, 16, Qt::AlignVCenter,
                   e.index.left(24));

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 20, y + 18, rect.width() - 50, 14, Qt::AlignVCenter,
                   e.category + " | " + e.type + " | " + QString::number(e.documents) + " docs");

        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 50, y + 2, 46, 14, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.size, 'f', 1) + " MB");
        show++;
    }
}

void PaperIndexManager::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto counts = categoryCounts();
    QStringList cats = {"Author", "Keyword", "Reference", "Institution"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 65, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperIndexManager::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total", QString::number(entries_.size()), QColor(59,130,246)},
        {"Active", QString::number(activeCount()), QColor(22,163,74)},
        {"Size (MB)", QString::number(totalSize(), 'f', 1), QColor(217,119,6)},
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

void PaperIndexManager::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Manage paper indexes"); return; }
    infoLabel_->setText(QString("%1 indexes | %2 active | %3 MB")
        .arg(entries_.size()).arg(activeCount())
        .arg(totalSize(), 0, 'f', 1));
}

void PaperIndexManager::onRebuild() {
    entries_.clear();
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    QStringList cats = {"Author", "Keyword", "Reference", "Institution"};
    QStringList types = {"B-Tree", "Hash", "Inverted", "Trie", "Bitmap"};
    int n = 4 + QRandomGenerator::global()->bounded(5);

    for (int i = 0; i < n; ++i) {
        IndexEntry e;
        e.id = i + 1;
        e.index = QString("idx_%1").arg(i + 1);
        e.category = cats[QRandomGenerator::global()->bounded(cats.size())];
        e.type = types[QRandomGenerator::global()->bounded(types.size())];
        e.size = QRandomGenerator::global()->bounded(10000) / 100.0;
        e.documents = QRandomGenerator::global()->bounded(500) + 10;
        e.active = QRandomGenerator::global()->bounded(5) != 0;
        e.color = palette[i % 5];
        entries_.append(e);
        emit indexRebuilt(e.id, e.size);
    }

    saveSettings();
    updateInfo();
    update();
}

void PaperIndexManager::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Manage paper indexes");
    update();
}

void PaperIndexManager::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        IndexEntry e;
        e.id = settings_.value("id").toInt();
        e.index = settings_.value("index").toString();
        e.category = settings_.value("category").toString();
        e.type = settings_.value("type").toString();
        e.size = settings_.value("size").toReal();
        e.documents = settings_.value("documents").toInt();
        e.active = settings_.value("active").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperIndexManager::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("index", entries_[i].index);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("type", entries_[i].type);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("documents", entries_[i].documents);
        settings_.setValue("active", entries_[i].active);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
