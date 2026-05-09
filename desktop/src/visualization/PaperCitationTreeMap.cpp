#include "visualization/PaperCitationTreeMap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <algorithm>

PaperCitationTreeMap::PaperCitationTreeMap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationTreeMap")
{
    setupUI();
    loadSettings();
}

void PaperCitationTreeMap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCitationTreeMap::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationTreeMap::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Visualize citation treemap");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationTreeMap::addNode(const TreeMapNode& node) {
    nodes_.append(node);
    saveSettings();
    updateInfo();
    emit mapUpdated(nodes_.size());
    update();
}

QList<TreeMapNode> PaperCitationTreeMap::nodes() const { return nodes_; }

QMap<QString, qreal> PaperCitationTreeMap::categoryWeights() const {
    QMap<QString, qreal> weights;
    for (const auto& n : nodes_) weights[n.category] += n.weight;
    return weights;
}

qreal PaperCitationTreeMap::totalWeight() const {
    qreal t = 0;
    for (const auto& n : nodes_) t += n.weight;
    return t;
}

void PaperCitationTreeMap::onGenerate() {
    bool ok;
    int count = QInputDialog::getInt(this, "Generate", "Number of fields:", 8, 3, 15, 1, &ok);
    if (!ok) return;

    nodes_.clear();
    QStringList categories = {"ML/AI", "NLP", "Vision", "Systems", "Theory", "Security", "Graphics", "HCI"};
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                          QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                          QColor(14,165,233), QColor(249,115,22)};

    for (int i = 0; i < qMin(count, categories.size()); ++i) {
        TreeMapNode n;
        n.id = i + 1;
        n.label = categories[i];
        n.category = categories[i];
        n.weight = 10 + QRandomGenerator::global()->bounded(90);
        n.color = catColors[i];
        nodes_.append(n);
    }
    saveSettings();
    updateInfo();
    emit mapUpdated(nodes_.size());
    update();
}

void PaperCitationTreeMap::onClear() {
    nodes_.clear();
    saveSettings();
    infoLabel_->setText("Visualize citation treemap");
    update();
}

void PaperCitationTreeMap::computeLayout(const QRectF& area, QList<TreeMapNode>& nodeList) {
    if (nodeList.isEmpty()) return;
    qreal totalW = 0;
    for (const auto& n : nodeList) totalW += n.weight;
    if (totalW <= 0) return;

    qreal offset = 0;
    bool vertical = area.width() >= area.height();

    for (auto& n : nodeList) {
        qreal fraction = n.weight / totalW;
        if (vertical) {
            n.rect = QRectF(area.x() + offset, area.y(), area.width() * fraction - 2, area.height() - 2);
            offset += area.width() * fraction;
        } else {
            n.rect = QRectF(area.x(), area.y() + offset, area.width() - 2, area.height() * fraction - 2);
            offset += area.height() * fraction;
        }
    }
}

void PaperCitationTreeMap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (nodes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Visualize citation treemap");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Tree Map");

    int w = width(), h = height();
    drawTreeMap(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationTreeMap::drawTreeMap(QPainter& p, const QRect& rect) {
    QList<TreeMapNode> sorted = nodes_;
    std::sort(sorted.begin(), sorted.end(),
              [](const TreeMapNode& a, const TreeMapNode& b) { return a.weight > b.weight; });
    computeLayout(QRectF(rect.x(), rect.y(), rect.width(), rect.height()), sorted);

    for (const auto& n : sorted) {
        p.setPen(QPen(Qt::white, 2));
        p.setBrush(n.color);
        p.drawRoundedRect(n.rect.toRect(), 4, 4);

        if (n.rect.width() > 50 && n.rect.height() > 30) {
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", qMin(10, static_cast<int>(n.rect.height() / 3))));
            p.drawText(n.rect.toRect(), Qt::AlignCenter,
                       n.label + "\n" + QString::number(static_cast<int>(n.weight)));
        }
    }
}

void PaperCitationTreeMap::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "By Category");

    auto weights = categoryWeights();
    QList<QString> cats = weights.keys();
    int maxShow = qMin(8, cats.size());
    int itemH = qMin(24, (rect.height() - 30) / qMax(maxShow, 1));

    qreal maxW = 1;
    for (const auto& w : weights) maxW = qMax(maxW, w);

    for (int i = 0; i < maxShow; ++i) {
        int y = rect.y() + 22 + i * (itemH + 3);
        int barW = static_cast<int>((weights[cats[i]] / maxW) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + itemH - 2, 65, itemH, Qt::AlignRight | Qt::AlignVCenter, cats[i].left(10));

        p.setPen(Qt::NoPen);
        p.setBrush(nodes_[i].color);
        p.drawRoundedRect(rect.x() + 70, y, barW, itemH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + itemH - 2, QString::number(static_cast<int>(weights[cats[i]])));
    }
}

void PaperCitationTreeMap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Fields", QString::number(nodes_.size()), QColor(59,130,246)},
        {"Total", QString::number(static_cast<int>(totalWeight())), QColor(16,185,129)},
        {"Categories", QString::number(categoryWeights().size()), QColor(245,158,11)},
        {"Avg Weight", QString::number(nodes_.isEmpty() ? 0 : totalWeight() / nodes_.size(), 'f', 0), QColor(139,92,246)}
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

void PaperCitationTreeMap::updateInfo() {
    if (nodes_.isEmpty()) { infoLabel_->setText("Visualize citation treemap"); return; }
    infoLabel_->setText(QString("%1 fields | total: %2")
        .arg(nodes_.size()).arg(static_cast<int>(totalWeight())));
}

void PaperCitationTreeMap::loadSettings() {
    int size = settings_.beginReadArray("nodes");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TreeMapNode n;
        n.id = settings_.value("id").toInt();
        n.label = settings_.value("label").toString();
        n.weight = settings_.value("weight").toDouble();
        n.category = settings_.value("category").toString();
        n.color = QColor(settings_.value("color").toString());
        nodes_.append(n);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationTreeMap::saveSettings() {
    settings_.beginWriteArray("nodes");
    for (int i = 0; i < nodes_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", nodes_[i].id);
        settings_.setValue("label", nodes_[i].label);
        settings_.setValue("weight", nodes_[i].weight);
        settings_.setValue("category", nodes_[i].category);
        settings_.setValue("color", nodes_[i].color.name());
    }
    settings_.endArray();
}
