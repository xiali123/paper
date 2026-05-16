#include "visualization/PaperSankeyDiagram.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSankeyDiagram::PaperSankeyDiagram(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SankeyDiagram")
{
    setupUI();
    loadSettings();
}

void PaperSankeyDiagram::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperSankeyDiagram::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Layer:"));
    layerCombo_ = new QComboBox();
    layerCombo_->addItems({"All", "Source", "Process", "Output"});
    toolbar->addWidget(layerCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSankeyDiagram::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter flow dataset...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate Sankey diagram");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperSankeyDiagram::addEntry(const SankeyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sankeyGenerated(entry.id, entry.flow);
    update();
}

QList<SankeyEntry> PaperSankeyDiagram::entries() const { return entries_; }

qreal PaperSankeyDiagram::totalFlow() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.flow;
    return t;
}

int PaperSankeyDiagram::majorCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.major) c++;
    return c;
}

QMap<QString, int> PaperSankeyDiagram::layerCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.layer]++;
    return counts;
}

void PaperSankeyDiagram::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList sources = {"Search", "Recommend", "Citation", "Feed"};
    QStringList targets = {"Abstract", "PDF", "Reference", "Bookmark"};
    QStringList categories = {"discovery", "reading", "citing", "sharing"};
    QStringList layers = {"source", "process", "output"};
    entries_.clear();
    int count = 6 + QRandomGenerator::global()->bounded(6);
    for (int i = 0; i < count; ++i) {
        SankeyEntry e;
        e.id = entries_.size() + 1;
        e.source = sources[QRandomGenerator::global()->bounded(sources.size())];
        e.target = targets[QRandomGenerator::global()->bounded(targets.size())];
        e.flow = 10 + QRandomGenerator::global()->bounded(90);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.efficiency = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.papersFlow = static_cast<int>(e.flow);
        e.layer = layers[QRandomGenerator::global()->bounded(layers.size())];
        e.major = e.flow >= 60;
        e.color = e.major ? QColor(59,130,246) : QColor(16,185,129);
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit sankeyGenerated(entries_.size(), totalFlow());
    update();
    inputField_->clear();
}

void PaperSankeyDiagram::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate Sankey diagram");
    update();
}

void PaperSankeyDiagram::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate Sankey diagram");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Sankey Diagram");
    int w = width(), h = height();
    drawSankeyView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLayerLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSankeyDiagram::drawSankeyView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    qreal maxFlow = 1;
    for (const auto& e : entries_) maxFlow = qMax(maxFlow, e.flow);
    int leftX = rect.x() + 10;
    int rightX = rect.x() + rect.width() - 80;
    int y = rect.y() + 10;
    int spacing = qMin(30, (rect.height() - 20) / qMax(n, 1));
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int curY = y + i * spacing;
        int bandH = qMax(4, static_cast<int>((e.flow / maxFlow) * spacing * 0.8));
        int alpha = 80 + static_cast<int>((e.flow / maxFlow) * 120);
        QColor fill = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        QPainterPath path;
        path.moveTo(leftX, curY);
        path.cubicTo(leftX + (rightX - leftX) / 2, curY,
                     leftX + (rightX - leftX) / 2, curY + bandH,
                     rightX, curY + bandH / 2);
        path.cubicTo(leftX + (rightX - leftX) / 2, curY + bandH,
                     leftX + (rightX - leftX) / 2, curY,
                     leftX, curY + bandH);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(path);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(leftX - 2, curY + bandH + 2, e.source.left(8));
        p.drawText(rightX + 4, curY + bandH / 2 + 4, e.target.left(8));
    }
}

void PaperSankeyDiagram::drawLayerLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Layers");
    auto counts = layerCounts();
    QStringList layers = {"source", "process", "output"};
    QString labels[] = {"Source", "Process", "Output"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(layers[i]) ? counts[layers[i]] : 0;
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
                   QString::number(count) + " flows");
    }
}

void PaperSankeyDiagram::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Flows", QString::number(entries_.size()), QColor(59,130,246)},
        {"Major", QString::number(majorCount()), QColor(16,185,129)},
        {"Total Flow", QString::number(totalFlow(), 'f', 0), QColor(245,158,11)},
        {"Layers", QString::number(layerCounts().size()), QColor(139,92,246)}
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

void PaperSankeyDiagram::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate Sankey diagram"); return; }
    infoLabel_->setText(QString("%1 flows | %2 major | %3 total")
        .arg(entries_.size()).arg(majorCount()).arg(totalFlow(), 0, 'f', 0));
}

void PaperSankeyDiagram::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SankeyEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.flow = settings_.value("flow").toDouble();
        e.category = settings_.value("category").toString();
        e.efficiency = settings_.value("efficiency").toDouble();
        e.papersFlow = settings_.value("papersFlow").toInt();
        e.layer = settings_.value("layer").toString();
        e.major = settings_.value("major").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSankeyDiagram::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("flow", entries_[i].flow);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("efficiency", entries_[i].efficiency);
        settings_.setValue("papersFlow", entries_[i].papersFlow);
        settings_.setValue("layer", entries_[i].layer);
        settings_.setValue("major", entries_[i].major);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
