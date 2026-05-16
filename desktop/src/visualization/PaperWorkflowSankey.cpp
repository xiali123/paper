#include "visualization/PaperWorkflowSankey.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>

PaperWorkflowSankey::PaperWorkflowSankey(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "WorkflowSankey")
{
    setupUI();
    loadSettings();
}

void PaperWorkflowSankey::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperWorkflowSankey::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperWorkflowSankey::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Visualize research workflow");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperWorkflowSankey::addNode(const SankeyNode& node) {
    nodes_.append(node);
    saveSettings();
    updateInfo();
    emit flowUpdated(totalFlow());
    update();
}

QList<SankeyNode> PaperWorkflowSankey::nodes() const { return nodes_; }

QMap<QString, qreal> PaperWorkflowSankey::typeFlows() const {
    QMap<QString, qreal> flows;
    for (const auto& n : nodes_) flows[n.type] += n.flow;
    return flows;
}

qreal PaperWorkflowSankey::totalFlow() const {
    qreal t = 0;
    for (const auto& n : nodes_) t += n.flow;
    return t;
}

void PaperWorkflowSankey::onGenerate() {
    nodes_.clear();
    QStringList sources = {"Search", "DB Query", "Recommendation", "Citation"};
    QStringList processes = {"Filter", "Rank", "Deduplicate", "Classify"};
    QStringList outputs = {"Reading List", "Export", "Report", "Archive"};

    for (const auto& s : sources) {
        SankeyNode n;
        n.id = nodes_.size() + 1;
        n.label = s;
        n.type = "source";
        n.flow = 10 + QRandomGenerator::global()->bounded(40);
        n.color = QColor(59,130,246);
        nodes_.append(n);
    }
    for (const auto& p : processes) {
        SankeyNode n;
        n.id = nodes_.size() + 1;
        n.label = p;
        n.type = "process";
        n.flow = 5 + QRandomGenerator::global()->bounded(30);
        n.color = QColor(245,158,11);
        nodes_.append(n);
    }
    for (const auto& o : outputs) {
        SankeyNode n;
        n.id = nodes_.size() + 1;
        n.label = o;
        n.type = "output";
        n.flow = 3 + QRandomGenerator::global()->bounded(20);
        n.color = QColor(16,185,129);
        nodes_.append(n);
    }
    saveSettings();
    updateInfo();
    emit flowUpdated(totalFlow());
    update();
}

void PaperWorkflowSankey::onClear() {
    nodes_.clear();
    saveSettings();
    infoLabel_->setText("Visualize research workflow");
    update();
}

void PaperWorkflowSankey::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (nodes_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Visualize research workflow");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Workflow Sankey");

    int w = width(), h = height();
    drawSankeyDiagram(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFlowBars(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperWorkflowSankey::drawSankeyDiagram(QPainter& p, const QRect& rect) {
    QList<SankeyNode*> sources, processes, outputs;
    for (auto& n : nodes_) {
        if (n.type == "source") sources.append(&n);
        else if (n.type == "process") processes.append(&n);
        else outputs.append(&n);
    }

    qreal maxFlow = 1;
    for (const auto& n : nodes_) maxFlow = qMax(maxFlow, n.flow);

    int colW = rect.width() / 3;
    int barMaxH = qMin(60, (rect.height() - 20) / 4);

    auto drawColumn = [&](QList<SankeyNode*>& list, int col, const QString& title) {
        int x = rect.x() + col * colW + 10;
        qreal totalF = 0;
        for (const auto* n : list) totalF += n.flow;
        int yOffset = rect.y() + 10;

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(x, yOffset - 2, colW - 20, 14, Qt::AlignCenter, title);
        yOffset += 16;

        for (auto* n : list) {
            qreal frac = totalF > 0 ? n->flow / totalF : 0;
            int barH = qMax(14, static_cast<int>(frac * (rect.height() - 40)));

            p.setPen(Qt::NoPen);
            p.setBrush(n->color);
            p.drawRoundedRect(x, yOffset, colW - 20, barH, 4, 4);

            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(x + 4, yOffset + 2, colW - 28, barH - 4, Qt::AlignVCenter,
                       n->label + " (" + QString::number(static_cast<int>(n->flow)) + ")");

            yOffset += barH + 3;
        }
    };

    drawColumn(sources, 0, "Sources");
    drawColumn(processes, 1, "Process");
    drawColumn(outputs, 2, "Outputs");

    for (int i = 0; i < sources.size(); ++i) {
        int target = i % processes.size();
        int x1 = rect.x() + colW - 10;
        int x2 = rect.x() + colW + 10;
        int y1 = rect.y() + 30 + i * 20;
        int y2 = rect.y() + 30 + target * 20;

        QPainterPath path;
        path.moveTo(x1, y1);
        path.cubicTo(x1 + colW / 3, y1, x2 - colW / 3, y2, x2, y2);
        p.setPen(QPen(QColor(203, 213, 225), 1));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }
}

void PaperWorkflowSankey::drawFlowBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Flow Distribution");

    auto flows = typeFlows();
    QStringList types = {"source", "process", "output"};
    QString labels[] = {"Sources", "Process", "Outputs"};
    QColor colors[] = {QColor(59,130,246), QColor(245,158,11), QColor(16,185,129)};

    qreal maxF = 1;
    for (const auto& f : flows) maxF = qMax(maxF, f);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        qreal flow = flows.contains(types[i]) ? flows[types[i]] : 0;
        int barW = static_cast<int>((flow / maxF) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 60, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 3, QString::number(static_cast<int>(flow)));
    }
}

void PaperWorkflowSankey::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Nodes", QString::number(nodes_.size()), QColor(59,130,246)},
        {"Total Flow", QString::number(static_cast<int>(totalFlow())), QColor(16,185,129)},
        {"Types", QString::number(typeFlows().size()), QColor(245,158,11)},
        {"Avg Flow", QString::number(nodes_.isEmpty() ? 0 : totalFlow() / nodes_.size(), 'f', 0), QColor(139,92,246)}
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

void PaperWorkflowSankey::updateInfo() {
    if (nodes_.isEmpty()) { infoLabel_->setText("Visualize research workflow"); return; }
    infoLabel_->setText(QString("%1 nodes | flow: %2")
        .arg(nodes_.size()).arg(static_cast<int>(totalFlow())));
}

void PaperWorkflowSankey::loadSettings() {
    int size = settings_.beginReadArray("nodes");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SankeyNode n;
        n.id = settings_.value("id").toInt();
        n.label = settings_.value("label").toString();
        n.type = settings_.value("type").toString();
        n.flow = settings_.value("flow").toDouble();
        n.color = QColor(settings_.value("color").toString());
        nodes_.append(n);
    }
    settings_.endArray();
    updateInfo();
}

void PaperWorkflowSankey::saveSettings() {
    settings_.beginWriteArray("nodes");
    for (int i = 0; i < nodes_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", nodes_[i].id);
        settings_.setValue("label", nodes_[i].label);
        settings_.setValue("type", nodes_[i].type);
        settings_.setValue("flow", nodes_[i].flow);
        settings_.setValue("color", nodes_[i].color.name());
    }
    settings_.endArray();
}
