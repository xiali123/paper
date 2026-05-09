#include "visualization/PaperCitationNetworkD3.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperCitationNetworkD3::PaperCitationNetworkD3(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationNetworkD3")
{
    setupUI();
    loadSettings();
}

void PaperCitationNetworkD3::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCitationNetworkD3::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationNetworkD3::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();

    searchField_ = new QLineEdit();
    searchField_->setPlaceholderText("Search papers...");
    searchField_->setStyleSheet("QLineEdit { padding: 4px; border: 1px solid #cbd5e1; border-radius: 4px; max-width: 160px; }");
    toolbar->addWidget(searchField_);
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Visualize citation network");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationNetworkD3::addNode(const NetworkNode& node) {
    nodes_.append(node);
    saveSettings();
    updateInfo();
    emit networkUpdated(nodes_.size());
    update();
}

QList<NetworkNode> PaperCitationNetworkD3::nodes() const { return nodes_; }

QMap<QString, int> PaperCitationNetworkD3::clusterCounts() const {
    QMap<QString, int> counts;
    for (const auto& n : nodes_) counts[n.cluster]++;
    return counts;
}

qreal PaperCitationNetworkD3::avgCentrality() const {
    if (nodes_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& n : nodes_) sum += n.centrality;
    return sum / nodes_.size();
}

int PaperCitationNetworkD3::totalDegree() const {
    int t = 0;
    for (const auto& n : nodes_) t += n.degree;
    return t;
}

void PaperCitationNetworkD3::onGenerate() {
    nodes_.clear();
    QStringList titles = {"Deep Learning", "Transformers", "GANs", "BERT", "GPT",
                          "ResNet", "Attention", "VAE", "Federated Learning", "Graph NN",
                          "Meta-Learning", "Contrastive Learning"};
    QStringList clusters = {"DL", "NLP", "CV", "RL"};
    QColor clusterColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(245,158,11)};

    for (int i = 0; i < titles.size(); ++i) {
        NetworkNode n;
        n.id = nodes_.size() + 1;
        n.paperTitle = titles[i];
        n.citationCount = 50 + QRandomGenerator::global()->bounded(950);
        n.year = 2017 + QRandomGenerator::global()->bounded(8);
        n.centrality = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
        int cIdx = QRandomGenerator::global()->bounded(clusters.size());
        n.cluster = clusters[cIdx];
        n.degree = 2 + QRandomGenerator::global()->bounded(10);
        n.color = clusterColors[cIdx];
        nodes_.append(n);
    }
    saveSettings();
    updateInfo();
    emit networkUpdated(nodes_.size());
    update();
}

void PaperCitationNetworkD3::onClear() {
    nodes_.clear();
    saveSettings();
    infoLabel_->setText("Visualize citation network");
    update();
}

void PaperCitationNetworkD3::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (nodes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Visualize citation network");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Network");

    int w = width(), h = height();
    drawNetworkView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawClusterChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationNetworkD3::drawNetworkView(QPainter& p, const QRect& rect) {
    int show = qMin(12, nodes_.size());
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int radius = qMin(rect.width(), rect.height()) / 2 - 30;

    QList<QPointF> positions;
    for (int i = 0; i < show; ++i) {
        qreal angle = (2 * M_PI * i / show) - M_PI / 2;
        qreal r = radius * (0.4 + nodes_[i].centrality * 0.6);
        qreal px = cx + r * std::cos(angle);
        qreal py = cy + r * std::sin(angle);
        positions << QPointF(px, py);
    }

    for (int i = 0; i < positions.size(); ++i) {
        int deg = qMin(3, nodes_[i].degree / 2);
        for (int j = 1; j <= deg && (i + j) < positions.size(); ++j) {
            p.setPen(QPen(QColor(226, 232, 240), 1));
            p.drawLine(positions[i], positions[(i + j) % positions.size()]);
        }
    }

    qreal maxCite = 1;
    for (const auto& n : nodes_) maxCite = qMax(maxCite, static_cast<qreal>(n.citationCount));

    for (int i = 0; i < show; ++i) {
        const auto& n = nodes_[i];
        int nodeR = 6 + static_cast<int>((n.citationCount / maxCite) * 14);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(n.color.red(), n.color.green(), n.color.blue(), 50));
        p.drawEllipse(positions[i], nodeR + 5, nodeR + 5);

        p.setBrush(n.color);
        p.drawEllipse(positions[i], nodeR, nodeR);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(QPointF(positions[i].x() - 18, positions[i].y() + nodeR + 10),
                   n.paperTitle.left(10));
    }
}

void PaperCitationNetworkD3::drawClusterChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Clusters");

    auto counts = clusterCounts();
    int total = nodes_.size();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    QStringList clusters = {"DL", "NLP", "CV", "RL"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(239,68,68), QColor(245,158,11)};

    qreal startAngle = 0;
    for (int i = 0; i < 4; ++i) {
        int count = counts.contains(clusters[i]) ? counts[clusters[i]] : 0;
        qreal span = (static_cast<qreal>(count) / total) * 360;
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

void PaperCitationNetworkD3::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Papers", QString::number(nodes_.size()), QColor(59,130,246)},
        {"Connections", QString::number(totalDegree()), QColor(16,185,129)},
        {"Clusters", QString::number(clusterCounts().size()), QColor(245,158,11)},
        {"Avg Centrality", QString::number(avgCentrality() * 100, 'f', 0) + "%", QColor(139,92,246)}
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

void PaperCitationNetworkD3::updateInfo() {
    if (nodes_.isEmpty()) { infoLabel_->setText("Visualize citation network"); return; }
    infoLabel_->setText(QString("%1 papers | %2 connections | %3 clusters")
        .arg(nodes_.size()).arg(totalDegree()).arg(clusterCounts().size()));
}

void PaperCitationNetworkD3::loadSettings() {
    int size = settings_.beginReadArray("nodes");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NetworkNode n;
        n.id = settings_.value("id").toInt();
        n.paperTitle = settings_.value("paperTitle").toString();
        n.citationCount = settings_.value("citationCount").toInt();
        n.year = settings_.value("year").toInt();
        n.centrality = settings_.value("centrality").toDouble();
        n.cluster = settings_.value("cluster").toString();
        n.degree = settings_.value("degree").toInt();
        n.color = QColor(settings_.value("color").toString());
        nodes_.append(n);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationNetworkD3::saveSettings() {
    settings_.beginWriteArray("nodes");
    for (int i = 0; i < nodes_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", nodes_[i].id);
        settings_.setValue("paperTitle", nodes_[i].paperTitle);
        settings_.setValue("citationCount", nodes_[i].citationCount);
        settings_.setValue("year", nodes_[i].year);
        settings_.setValue("centrality", nodes_[i].centrality);
        settings_.setValue("cluster", nodes_[i].cluster);
        settings_.setValue("degree", nodes_[i].degree);
        settings_.setValue("color", nodes_[i].color.name());
    }
    settings_.endArray();
}
