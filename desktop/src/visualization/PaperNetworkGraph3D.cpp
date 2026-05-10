#include "visualization/PaperNetworkGraph3D.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperNetworkGraph3D::PaperNetworkGraph3D(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NetworkGraph3D")
{
    setupUI();
    loadSettings();
}

void PaperNetworkGraph3D::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperNetworkGraph3D::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Authors", "Topics", "Institutions"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNetworkGraph3D::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter graph dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate 3D network graph");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperNetworkGraph3D::addEntry(const GraphNode3D& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit graphGenerated(entry.id, entry.connections);
    update();
}

QList<GraphNode3D> PaperNetworkGraph3D::entries() const { return entries_; }

int PaperNetworkGraph3D::hubCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.hub) c++;
    return c;
}

qreal PaperNetworkGraph3D::avgConnections() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.connections;
    return sum / entries_.size();
}

QMap<QString, int> PaperNetworkGraph3D::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperNetworkGraph3D::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"authors", "topics", "institutions"};
    QStringList labels = {"Neural Net", "Deep Learning", "NLP", "Vision", "RL", "GAN", "Transform", "CNN", "RNN", "BERT"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 8 + QRandomGenerator::global()->bounded(8);
    for (int i = 0; i < count; ++i) {
        GraphNode3D e;
        e.id = entries_.size() + 1;
        e.label = labels[i % labels.size()];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.x = QRandomGenerator::global()->bounded(1000) / 1000.0;
        e.y = QRandomGenerator::global()->bounded(1000) / 1000.0;
        e.z = QRandomGenerator::global()->bounded(1000) / 1000.0;
        e.connections = 1 + QRandomGenerator::global()->bounded(20);
        e.weight = 0.5 + QRandomGenerator::global()->bounded(50) / 10.0;
        e.size = e.connections / 20.0;
        e.hub = e.connections >= 12;
        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
        int ci = categories.indexOf(e.category);
        e.color = catColors[ci >= 0 ? ci : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit graphGenerated(entries_.size(), static_cast<int>(avgConnections()));
    update();
    inputField_->clear();
}

void PaperNetworkGraph3D::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate 3D network graph");
    update();
}

void PaperNetworkGraph3D::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate 3D network graph");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Network Graph 3D");
    int w = width(), h = height();
    drawGraphView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperNetworkGraph3D::drawGraphView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    qreal maxConn = 1;
    for (const auto& e : entries_) maxConn = qMax(maxConn, static_cast<qreal>(e.connections));
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        qreal projX = e.x * 0.8 + e.z * 0.2;
        qreal projY = e.y * 0.8 + e.z * 0.15;
        int x = rect.x() + static_cast<int>(projX * rect.width());
        int y = rect.y() + static_cast<int>(projY * rect.height());
        int radius = qMax(4, static_cast<int>((e.connections / maxConn) * 16));
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(x - radius, y - radius, radius * 2, radius * 2);
        if (e.hub) {
            p.setPen(QPen(e.color, 1));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(x - radius - 3, y - radius - 3, (radius + 3) * 2, (radius + 3) * 2);
        }
        if (radius >= 6) {
            p.setPen(QColor(15, 23, 42));
            p.setFont(QFont("Arial", qMax(6, radius / 2)));
            p.drawText(x - radius, y - radius - 2, radius * 2, radius * 2, Qt::AlignCenter, e.label.left(4));
        }
    }
}

void PaperNetworkGraph3D::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"authors", "topics", "institutions"};
    QString labels[] = {"Authors", "Topics", "Institutions"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " nodes");
    }
}

void PaperNetworkGraph3D::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Nodes", QString::number(entries_.size()), QColor(59,130,246)},
        {"Hubs", QString::number(hubCount()), QColor(16,185,129)},
        {"Avg Conn", QString::number(avgConnections(), 'f', 1), QColor(245,158,11)},
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

void PaperNetworkGraph3D::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate 3D network graph"); return; }
    infoLabel_->setText(QString("%1 nodes | %2 hubs | %3 avg conn")
        .arg(entries_.size()).arg(hubCount()).arg(avgConnections(), 0, 'f', 1));
}

void PaperNetworkGraph3D::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        GraphNode3D e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.x = settings_.value("x").toDouble();
        e.y = settings_.value("y").toDouble();
        e.z = settings_.value("z").toDouble();
        e.size = settings_.value("size").toDouble();
        e.connections = settings_.value("connections").toInt();
        e.weight = settings_.value("weight").toDouble();
        e.hub = settings_.value("hub").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNetworkGraph3D::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("x", entries_[i].x);
        settings_.setValue("y", entries_[i].y);
        settings_.setValue("z", entries_[i].z);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("connections", entries_[i].connections);
        settings_.setValue("weight", entries_[i].weight);
        settings_.setValue("hub", entries_[i].hub);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
