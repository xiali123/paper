#include "visualization/PaperCitationFlowDiagram.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperCitationFlowDiagram::PaperCitationFlowDiagram(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationFlowDiagram")
{
    setupUI();
    loadSettings();
}

void PaperCitationFlowDiagram::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCitationFlowDiagram::onGenerate);
    toolbar->addWidget(generateBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationFlowDiagram::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Citation flow diagram");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationFlowDiagram::addEntry(const FlowEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit flowUpdated(entries_.size());
    update();
}

QList<FlowEntry> PaperCitationFlowDiagram::entries() const { return entries_; }

QMap<QString, int> PaperCitationFlowDiagram::typeCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.flowType]++;
    return counts;
}

qreal PaperCitationFlowDiagram::totalWeight() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.weight;
    return t;
}

qreal PaperCitationFlowDiagram::avgStrength() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.strength;
    return sum / entries_.size();
}

void PaperCitationFlowDiagram::onGenerate() {
    entries_.clear();
    QStringList sources = {"Field A", "Field B", "Field C"};
    QStringList targets = {"Paper 1", "Paper 2", "Paper 3", "Paper 4", "Paper 5"};
    QStringList types = {"direct", "indirect", "self-cite"};
    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};

    for (const auto& src : sources) {
        int links = 2 + QRandomGenerator::global()->bounded(3);
        for (int j = 0; j < links; ++j) {
            FlowEntry e;
            e.id = entries_.size() + 1;
            e.source = src;
            e.target = targets[QRandomGenerator::global()->bounded(targets.size())];
            e.weight = 1 + QRandomGenerator::global()->bounded(20);
            int tIdx = QRandomGenerator::global()->bounded(types.size());
            e.flowType = types[tIdx];
            e.year = 2020 + QRandomGenerator::global()->bounded(6);
            e.strength = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
            e.label = QString::number(static_cast<int>(e.weight));
            e.color = typeColors[tIdx];
            entries_.append(e);
        }
    }
    saveSettings();
    updateInfo();
    emit flowUpdated(entries_.size());
    update();
}

void PaperCitationFlowDiagram::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Citation flow diagram");
    update();
}

void PaperCitationFlowDiagram::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Citation flow diagram");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Flow Diagram");

    int w = width(), h = height();
    drawFlowView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTypeChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationFlowDiagram::drawFlowView(QPainter& p, const QRect& rect) {
    QSet<QString> uniqueSources, uniqueTargets;
    for (const auto& e : entries_) {
        uniqueSources.insert(e.source);
        uniqueTargets.insert(e.target);
    }

    QStringList srcList = uniqueSources.values();
    QStringList tgtList = uniqueTargets.values();

    qreal maxWeight = 1;
    for (const auto& e : entries_) maxWeight = qMax(maxWeight, e.weight);

    int leftX = rect.x() + 10;
    int rightX = rect.x() + rect.width() - 80;
    int srcSpacing = rect.height() / qMax(srcList.size(), 1);
    int tgtSpacing = rect.height() / qMax(tgtList.size(), 1);

    for (int i = 0; i < srcList.size(); ++i) {
        int y = rect.y() + 20 + i * srcSpacing;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(59,130,246));
        p.drawRoundedRect(leftX, y, 60, qMin(srcSpacing - 4, 30), 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(leftX + 4, y + 2, 52, qMin(srcSpacing - 8, 26), Qt::AlignCenter, srcList[i].left(8));
    }

    for (int i = 0; i < tgtList.size(); ++i) {
        int y = rect.y() + 20 + i * tgtSpacing;
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(16,185,129));
        p.drawRoundedRect(rightX, y, 60, qMin(tgtSpacing - 4, 24), 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 6, QFont::Bold));
        p.drawText(rightX + 2, y + 2, 56, qMin(tgtSpacing - 8, 20), Qt::AlignCenter, tgtList[i].left(8));
    }

    for (const auto& e : entries_) {
        int srcIdx = srcList.indexOf(e.source);
        int tgtIdx = tgtList.indexOf(e.target);
        if (srcIdx < 0 || tgtIdx < 0) continue;

        int y1 = rect.y() + 20 + srcIdx * srcSpacing + (qMin(srcSpacing - 4, 30)) / 2;
        int y2 = rect.y() + 20 + tgtIdx * tgtSpacing + (qMin(tgtSpacing - 4, 24)) / 2;
        int thickness = 1 + static_cast<int>((e.weight / maxWeight) * 4);

        QPainterPath path;
        path.moveTo(leftX + 60, y1);
        int midX = (leftX + 60 + rightX) / 2;
        path.cubicTo(midX, y1, midX, y2, rightX, y2);

        p.setPen(QPen(QColor(e.color.red(), e.color.green(), e.color.blue(), 120), thickness));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }
}

void PaperCitationFlowDiagram::drawTypeChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Flow Types");

    auto counts = typeCounts();
    QStringList types = {"direct", "indirect", "self-cite"};
    QString labels[] = {"Direct", "Indirect", "Self-cite"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};

    int total = entries_.size();
    int pieW = qMin(rect.width(), rect.height() - 50);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 25 + pieW / 2;

    qreal startAngle = 0;
    for (int i = 0; i < 3; ++i) {
        int count = counts.contains(types[i]) ? counts[types[i]] : 0;
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

void PaperCitationFlowDiagram::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Flows", QString::number(entries_.size()), QColor(59,130,246)},
        {"Weight", QString::number(static_cast<int>(totalWeight())), QColor(16,185,129)},
        {"Avg Str", QString::number(avgStrength() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Types", QString::number(typeCounts().size()), QColor(139,92,246)}
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

void PaperCitationFlowDiagram::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Citation flow diagram"); return; }
    infoLabel_->setText(QString("%1 flows | %2 weight | %3% avg str")
        .arg(entries_.size()).arg(static_cast<int>(totalWeight())).arg(avgStrength() * 100, 0, 'f', 0));
}

void PaperCitationFlowDiagram::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        FlowEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.weight = settings_.value("weight").toDouble();
        e.flowType = settings_.value("flowType").toString();
        e.year = settings_.value("year").toInt();
        e.strength = settings_.value("strength").toDouble();
        e.label = settings_.value("label").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationFlowDiagram::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("weight", entries_[i].weight);
        settings_.setValue("flowType", entries_[i].flowType);
        settings_.setValue("year", entries_[i].year);
        settings_.setValue("strength", entries_[i].strength);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
