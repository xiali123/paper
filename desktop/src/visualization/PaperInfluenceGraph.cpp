#include "visualization/PaperInfluenceGraph.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperInfluenceGraph::PaperInfluenceGraph(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "InfluenceGraph")
{
    setupUI();
    loadSettings();
}

void PaperInfluenceGraph::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Author");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperInfluenceGraph::onAdd);
    toolbar->addWidget(addBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperInfluenceGraph::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Map author influence");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperInfluenceGraph::addNode(const InfluenceNode& node) {
    nodes_.append(node);
    saveSettings();
    updateInfo();
    emit graphUpdated(nodes_.size());
    update();
}

QList<InfluenceNode> PaperInfluenceGraph::nodes() const { return nodes_; }

qreal PaperInfluenceGraph::averageInfluence() const {
    if (nodes_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& n : nodes_) sum += n.influence;
    return sum / nodes_.size();
}

int PaperInfluenceGraph::topInfluencer() const {
    if (nodes_.isEmpty()) return -1;
    qreal maxInf = 0;
    int id = -1;
    for (const auto& n : nodes_) {
        if (n.influence > maxInf) { maxInf = n.influence; id = n.id; }
    }
    return id;
}

void PaperInfluenceGraph::onAdd() {
    bool ok;
    QString author = QInputDialog::getText(this, "Add Author", "Author name:", QLineEdit::Normal, "", &ok);
    if (!ok || author.isEmpty()) return;
    QStringList fields = {"ML/AI", "NLP", "Computer Vision", "Systems", "Theory"};
    QString field = QInputDialog::getItem(this, "Add Author", "Field:", fields, 0, false, &ok);
    if (!ok) return;

    InfluenceNode n;
    n.id = nodes_.size() + 1;
    n.author = author;
    n.field = field;
    n.influence = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
    n.collaborators = QRandomGenerator::global()->bounded(50);
    n.papers = QRandomGenerator::global()->bounded(100);
    n.score = n.influence * 0.6 + n.papers / 100.0 * 0.4;

    if (n.score >= 0.8) n.color = QColor(16,185,129);
    else if (n.score >= 0.5) n.color = QColor(59,130,246);
    else if (n.score >= 0.3) n.color = QColor(245,158,11);
    else n.color = QColor(239,68,68);

    addNode(n);
}

void PaperInfluenceGraph::onClear() {
    nodes_.clear();
    selectedNode_ = -1;
    saveSettings();
    infoLabel_->setText("Map author influence");
    update();
}

void PaperInfluenceGraph::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (nodes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Map author influence");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Influence Graph");

    int w = width(), h = height();
    drawNetworkGraph(p, QRect(20, 50, w / 2 - 20, h / 2));
    drawInfluenceBars(p, QRect(20, h / 2 + 10, w - 40, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
}

void PaperInfluenceGraph::drawNetworkGraph(QPainter& p, const QRect& rect) {
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;
    int maxRadius = qMin(rect.width(), rect.height()) / 2 - 30;

    for (int i = 0; i < nodes_.size(); ++i) {
        const auto& n = nodes_[i];
        qreal angle = (2 * M_PI * i / nodes_.size()) - M_PI / 2;
        qreal dist = maxRadius * (0.4 + n.influence * 0.6);
        qreal px = cx + dist * std::cos(angle);
        qreal py = cy + dist * std::sin(angle);
        int nodeR = qBound(6, 4 + static_cast<int>(n.papers / 10.0), 18);

        if (i > 0) {
            const auto& prev = nodes_[i - 1];
            qreal prevAngle = (2 * M_PI * (i - 1) / nodes_.size()) - M_PI / 2;
            qreal prevDist = maxRadius * (0.4 + prev.influence * 0.6);
            qreal ppx = cx + prevDist * std::cos(prevAngle);
            qreal ppy = cy + prevDist * std::sin(prevAngle);
            p.setPen(QPen(QColor(226, 232, 240), 1));
            p.drawLine(QPointF(ppx, ppy), QPointF(px, py));
        }

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(n.color.red(), n.color.green(), n.color.blue(), 50));
        p.drawEllipse(QPointF(px, py), nodeR + 6, nodeR + 6);

        p.setBrush(n.color);
        p.drawEllipse(QPointF(px, py), nodeR, nodeR);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(QPointF(px - 20, py - nodeR - 4), n.author.left(12));
    }
}

void PaperInfluenceGraph::drawInfluenceBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Influence Scores");

    int show = qMin(10, nodes_.size());
    int barH = qMin(18, (rect.height() - 25) / show);

    for (int i = 0; i < show; ++i) {
        const auto& n = nodes_[i];
        int y = rect.y() + 20 + i * (barH + 3);
        int barW = static_cast<int>(n.influence * (rect.width() - 140));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 80, barH, Qt::AlignRight | Qt::AlignVCenter,
                   n.author.left(12));

        p.setPen(Qt::NoPen);
        p.setBrush(n.color);
        p.drawRoundedRect(rect.x() + 85, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 88 + barW, y + barH - 2,
                   QString::number(n.influence * 100, 'f', 0) + "%");
    }
}

void PaperInfluenceGraph::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Authors", QString::number(nodes_.size()), QColor(59,130,246)},
        {"Avg Influence", QString::number(averageInfluence() * 100, 'f', 0) + "%", QColor(16,185,129)},
        {"Top ID", QString::number(topInfluencer()), QColor(245,158,11)},
        {"Papers", QString::number([this]{ int t=0; for(const auto& n: nodes_) t+=n.papers; return t; }()), QColor(139,92,246)}
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

void PaperInfluenceGraph::updateInfo() {
    if (nodes_.isEmpty()) { infoLabel_->setText("Map author influence"); return; }
    infoLabel_->setText(QString("%1 authors | avg influence: %2% | top: #%3")
        .arg(nodes_.size()).arg(averageInfluence() * 100, 0, 'f', 0).arg(topInfluencer()));
}

void PaperInfluenceGraph::loadSettings() {
    int size = settings_.beginReadArray("nodes");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        InfluenceNode n;
        n.id = settings_.value("id").toInt();
        n.author = settings_.value("author").toString();
        n.field = settings_.value("field").toString();
        n.influence = settings_.value("influence").toDouble();
        n.collaborators = settings_.value("collaborators").toInt();
        n.papers = settings_.value("papers").toInt();
        n.score = settings_.value("score").toDouble();
        n.color = QColor(settings_.value("color").toString());
        nodes_.append(n);
    }
    settings_.endArray();
    updateInfo();
}

void PaperInfluenceGraph::saveSettings() {
    settings_.beginWriteArray("nodes");
    for (int i = 0; i < nodes_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", nodes_[i].id);
        settings_.setValue("author", nodes_[i].author);
        settings_.setValue("field", nodes_[i].field);
        settings_.setValue("influence", nodes_[i].influence);
        settings_.setValue("collaborators", nodes_[i].collaborators);
        settings_.setValue("papers", nodes_[i].papers);
        settings_.setValue("score", nodes_[i].score);
        settings_.setValue("color", nodes_[i].color.name());
    }
    settings_.endArray();
}
