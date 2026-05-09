#include "visualization/PaperKnowledgeGraphExplorer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperKnowledgeGraphExplorer::PaperKnowledgeGraphExplorer(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "KnowledgeGraphExplorer")
{
    setupUI();
    loadSettings();
}

void PaperKnowledgeGraphExplorer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Node");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperKnowledgeGraphExplorer::onAdd);
    toolbar->addWidget(addBtn_);

    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #16a34a; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperKnowledgeGraphExplorer::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperKnowledgeGraphExplorer::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();

    searchField_ = new QLineEdit();
    searchField_->setPlaceholderText("Search concepts...");
    searchField_->setStyleSheet("QLineEdit { padding: 4px; border: 1px solid #cbd5e1; border-radius: 4px; max-width: 160px; }");
    toolbar->addWidget(searchField_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Explore knowledge graph");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperKnowledgeGraphExplorer::addNode(const KnowledgeNode& node) {
    nodes_.append(node);
    saveSettings();
    updateInfo();
    emit graphUpdated(nodes_.size(), totalConnections());
    update();
}

QList<KnowledgeNode> PaperKnowledgeGraphExplorer::nodes() const { return nodes_; }

QMap<QString, int> PaperKnowledgeGraphExplorer::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& n : nodes_) counts[n.category]++;
    return counts;
}

qreal PaperKnowledgeGraphExplorer::avgWeight() const {
    if (nodes_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& n : nodes_) sum += n.weight;
    return sum / nodes_.size();
}

int PaperKnowledgeGraphExplorer::totalConnections() const {
    int t = 0;
    for (const auto& n : nodes_) t += n.connections;
    return t;
}

void PaperKnowledgeGraphExplorer::onAdd() {
    bool ok;
    QString concept = QInputDialog::getText(this, "Add Node", "Concept:", QLineEdit::Normal, "", &ok);
    if (!ok || concept.isEmpty()) return;

    KnowledgeNode n;
    n.id = nodes_.size() + 1;
    n.concept = concept;
    QStringList cats = {"method", "theory", "dataset", "metric", "application"};
    n.category = cats[QRandomGenerator::global()->bounded(cats.size())];
    n.weight = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
    n.connections = 1 + QRandomGenerator::global()->bounded(10);
    n.description = "Node for " + concept.left(15);

    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};
    int cIdx = cats.indexOf(n.category);
    n.color = catColors[qBound(0, cIdx, 4)];
    addNode(n);
}

void PaperKnowledgeGraphExplorer::onGenerate() {
    nodes_.clear();
    QStringList concepts = {"CNN", "RNN", "Transformer", "Attention", "BERT", "GPT",
                            "ResNet", "GAN", "VAE", "Diffusion", "RL", "Meta-Learn"};
    QStringList cats = {"method", "theory", "dataset", "metric", "application"};
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    for (const auto& c : concepts) {
        KnowledgeNode n;
        n.id = nodes_.size() + 1;
        n.concept = c;
        int cIdx = QRandomGenerator::global()->bounded(cats.size());
        n.category = cats[cIdx];
        n.weight = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        n.connections = 2 + QRandomGenerator::global()->bounded(8);
        n.description = "Knowledge about " + c;
        n.color = catColors[cIdx];
        nodes_.append(n);
    }
    saveSettings();
    updateInfo();
    emit graphUpdated(nodes_.size(), totalConnections());
    update();
}

void PaperKnowledgeGraphExplorer::onClear() {
    nodes_.clear();
    saveSettings();
    infoLabel_->setText("Explore knowledge graph");
    update();
}

void PaperKnowledgeGraphExplorer::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (nodes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Explore knowledge graph");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Knowledge Graph Explorer");

    int w = width(), h = height();
    drawGraphView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperKnowledgeGraphExplorer::drawGraphView(QPainter& p, const QRect& rect) {
    int show = qMin(12, nodes_.size());
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;

    QList<QPointF> positions;
    for (int i = 0; i < show; ++i) {
        qreal angle = (2 * M_PI * i / show) - M_PI / 2;
        qreal r = radius * (0.5 + nodes_[i].weight * 0.5);
        qreal px = cx + r * std::cos(angle);
        qreal py = cy + r * std::sin(angle);
        positions << QPointF(px, py);
    }

    for (int i = 0; i < positions.size(); ++i) {
        int connections = qMin(3, nodes_[i].connections);
        for (int j = 1; j <= connections && (i + j) < positions.size(); ++j) {
            p.setPen(QPen(QColor(226, 232, 240), 1));
            p.drawLine(positions[i], positions[i + j]);
        }
    }

    for (int i = 0; i < show; ++i) {
        const auto& n = nodes_[i];
        int nodeR = 8 + static_cast<int>(n.weight * 12);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(n.color.red(), n.color.green(), n.color.blue(), 60));
        p.drawEllipse(positions[i], nodeR + 4, nodeR + 4);

        p.setBrush(n.color);
        p.drawEllipse(positions[i], nodeR, nodeR);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QPointF(positions[i].x() - 20, positions[i].y() + nodeR + 10),
                   n.concept.left(8));
    }
}

void PaperKnowledgeGraphExplorer::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QStringList cats = {"method", "theory", "dataset", "metric", "application"};
    QString labels[] = {"Method", "Theory", "Dataset", "Metric", "Application"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(239,68,68), QColor(139,92,246)};

    int total = nodes_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        qreal span = (static_cast<qreal>(count) / qMax(total, 1)) * 360;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10, QFont::Bold));
    p.drawText(cx - 10, cy + 5, QString::number(total));
}

void PaperKnowledgeGraphExplorer::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Nodes", QString::number(nodes_.size()), QColor(59,130,246)},
        {"Connections", QString::number(totalConnections()), QColor(16,185,129)},
        {"Categories", QString::number(categoryCounts().size()), QColor(245,158,11)},
        {"Avg Weight", QString::number(avgWeight() * 100, 'f', 0) + "%", QColor(139,92,246)}
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

void PaperKnowledgeGraphExplorer::updateInfo() {
    if (nodes_.isEmpty()) { infoLabel_->setText("Explore knowledge graph"); return; }
    infoLabel_->setText(QString("%1 nodes | %2 connections | %3 categories")
        .arg(nodes_.size()).arg(totalConnections()).arg(categoryCounts().size()));
}

void PaperKnowledgeGraphExplorer::loadSettings() {
    int size = settings_.beginReadArray("nodes");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        KnowledgeNode n;
        n.id = settings_.value("id").toInt();
        n.concept = settings_.value("concept").toString();
        n.category = settings_.value("category").toString();
        n.weight = settings_.value("weight").toDouble();
        n.connections = settings_.value("connections").toInt();
        n.description = settings_.value("description").toString();
        n.color = QColor(settings_.value("color").toString());
        nodes_.append(n);
    }
    settings_.endArray();
    updateInfo();
}

void PaperKnowledgeGraphExplorer::saveSettings() {
    settings_.beginWriteArray("nodes");
    for (int i = 0; i < nodes_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", nodes_[i].id);
        settings_.setValue("concept", nodes_[i].concept);
        settings_.setValue("category", nodes_[i].category);
        settings_.setValue("weight", nodes_[i].weight);
        settings_.setValue("connections", nodes_[i].connections);
        settings_.setValue("description", nodes_[i].description);
        settings_.setValue("color", nodes_[i].color.name());
    }
    settings_.endArray();
}
