#include "reading/PaperReadingNexus.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperReadingNexus::PaperReadingNexus(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ReadingNexus")
{
    setupUI();
    loadSettings();
}

void PaperReadingNexus::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    connectBtn_ = new QPushButton("Connect");
    connectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(connectBtn_, &QPushButton::clicked, this, &PaperReadingNexus::onConnect);
    toolbar->addWidget(connectBtn_);
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Core", "Bridge", "Leaf", "Cluster"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter node name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperReadingNexus::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Connect reading nexus");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperReadingNexus::addEntry(const NexusEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit nodeLinked(entry.id, entry.centrality);
    update();
}

QList<NexusEntry> PaperReadingNexus::entries() const { return entries_; }

int PaperReadingNexus::hubCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.hub) c++;
    return c;
}

qreal PaperReadingNexus::avgCentrality() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.centrality;
    return sum / entries_.size();
}

QMap<QString, int> PaperReadingNexus::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperReadingNexus::onConnect() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Core", "Bridge", "Leaf", "Cluster"};
    QStringList links = {"Strong", "Moderate", "Weak", "Bidirectional", "Indirect"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int cIdx = categoryCombo_->currentIndex();
    int count = 3 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        NexusEntry e;
        e.id = entries_.size() + 1;
        e.node = text.left(8) + " node" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.link = links[QRandomGenerator::global()->bounded(links.size())];
        e.centrality = 1 + QRandomGenerator::global()->bounded(90) / 100.0;
        e.connections = 2 + QRandomGenerator::global()->bounded(20);
        e.hub = QRandomGenerator::global()->bounded(100) < 30;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperReadingNexus::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Connect reading nexus");
    update();
}

void PaperReadingNexus::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Connect reading nexus");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Reading Nexus");
    int w = width(), h = height();
    drawNexusGraph(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperReadingNexus::drawNexusGraph(QPainter& p, const QRect& rect) {
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
                   e.node.left(14) + (e.hub ? " [H]" : " [N]"));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + 10, y + 20, rect.width() / 2 - 10, 14, Qt::AlignVCenter,
                   e.category + " | " + e.link + " | " + QString::number(e.connections) + " links");
        int barW = static_cast<int>(e.centrality * (rect.width() / 2 - 20));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x() + rect.width() / 2, y + 10, barW, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(rect.x() + rect.width() / 2 + barW + 4, y + 22,
                   QString::number(e.centrality, 'f', 2) + "c " + QString::number(e.connections) + "n");
    }
}

void PaperReadingNexus::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Core", "Bridge", "Leaf", "Cluster"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(124,58,237)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 80));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x(), y + barH - 2, 50, barH, Qt::AlignRight | Qt::AlignVCenter, categories[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 55, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 58 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperReadingNexus::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Nodes", QString::number(entries_.size()), QColor(59,130,246)},
        {"Hubs", QString::number(hubCount()), QColor(22,163,74)},
        {"Avg Centrality", QString::number(avgCentrality(), 'f', 2), QColor(217,119,6)},
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

void PaperReadingNexus::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Connect reading nexus"); return; }
    infoLabel_->setText(QString("%1 nodes | %2 hubs | centrality %3")
        .arg(entries_.size()).arg(hubCount()).arg(avgCentrality(), 0, 'f', 2));
}

void PaperReadingNexus::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NexusEntry e;
        e.id = settings_.value("id").toInt();
        e.node = settings_.value("node").toString();
        e.category = settings_.value("category").toString();
        e.link = settings_.value("link").toString();
        e.centrality = settings_.value("centrality").toDouble();
        e.connections = settings_.value("connections").toInt();
        e.hub = settings_.value("hub").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperReadingNexus::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("node", entries_[i].node);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("link", entries_[i].link);
        settings_.setValue("centrality", entries_[i].centrality);
        settings_.setValue("connections", entries_[i].connections);
        settings_.setValue("hub", entries_[i].hub);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
