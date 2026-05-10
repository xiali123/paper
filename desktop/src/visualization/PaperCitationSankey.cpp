#include "visualization/PaperCitationSankey.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperCitationSankey::PaperCitationSankey(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "CitationSankey")
{
    setupUI();
    loadSettings();
}

void PaperCitationSankey::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperCitationSankey::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Field:"));
    fieldCombo_ = new QComboBox();
    fieldCombo_->addItems({"All", "ML", "NLP", "Vision", "Theory"});
    toolbar->addWidget(fieldCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperCitationSankey::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Citation Sankey diagram");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperCitationSankey::addEntry(const CitationSankeyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sankeyGenerated(entry.id, entry.weight);
    update();
}

QList<CitationSankeyEntry> PaperCitationSankey::entries() const { return entries_; }

qreal PaperCitationSankey::totalWeight() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.weight;
    return t;
}

qreal PaperCitationSankey::avgImpact() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.impact;
    return sum / entries_.size();
}

QMap<QString, int> PaperCitationSankey::fieldCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.field]++;
    return counts;
}

void PaperCitationSankey::onGenerate() {
    entries_.clear();
    QStringList sources = {"Research Area A", "Research Area B", "Research Area C"};
    QStringList targets = {"Paper X", "Paper Y", "Paper Z", "Paper W"};
    QStringList types = {"direct-cite", "indirect", "self-cite"};
    QStringList fields = {"ML", "NLP", "Vision", "Theory"};
    QColor typeColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};

    for (const auto& src : sources) {
        int links = 2 + QRandomGenerator::global()->bounded(3);
        for (int j = 0; j < links; ++j) {
            CitationSankeyEntry e;
            e.id = entries_.size() + 1;
            e.source = src;
            e.target = targets[QRandomGenerator::global()->bounded(targets.size())];
            e.weight = 2 + QRandomGenerator::global()->bounded(25);
            int tIdx = QRandomGenerator::global()->bounded(types.size());
            e.flowType = types[tIdx];
            e.year = 2019 + QRandomGenerator::global()->bounded(7);
            e.field = fields[QRandomGenerator::global()->bounded(fields.size())];
            e.impact = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
            e.label = QString::number(static_cast<int>(e.weight));
            e.color = typeColors[tIdx];
            entries_.append(e);
        }
    }
    saveSettings();
    updateInfo();
    emit sankeyGenerated(entries_.size(), totalWeight());
    update();
}

void PaperCitationSankey::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Citation Sankey diagram");
    update();
}

void PaperCitationSankey::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Citation Sankey diagram");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Citation Sankey");

    int w = width(), h = height();
    drawSankeyView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawFieldChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperCitationSankey::drawSankeyView(QPainter& p, const QRect& rect) {
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
    int rightX = rect.x() + rect.width() - 90;
    int srcSpacing = rect.height() / qMax(srcList.size(), 1);
    int tgtSpacing = rect.height() / qMax(tgtList.size(), 1);

    for (int i = 0; i < srcList.size(); ++i) {
        int y = rect.y() + 20 + i * srcSpacing;
        int barH = qMin(srcSpacing - 4, 28);
        qreal srcWeight = 0;
        for (const auto& e : entries_) if (e.source == srcList[i]) srcWeight += e.weight;
        int barW = 20 + static_cast<int>((srcWeight / (maxWeight * srcList.size())) * 40);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(59,130,246));
        p.drawRoundedRect(leftX, y, barW, barH, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 7, QFont::Bold));
        p.drawText(leftX + 2, y + 2, barW - 4, barH - 4, Qt::AlignCenter, srcList[i].left(5));
    }

    for (int i = 0; i < tgtList.size(); ++i) {
        int y = rect.y() + 20 + i * tgtSpacing;
        int barH = qMin(tgtSpacing - 4, 24);
        qreal tgtWeight = 0;
        for (const auto& e : entries_) if (e.target == tgtList[i]) tgtWeight += e.weight;
        int barW = 20 + static_cast<int>((tgtWeight / (maxWeight * tgtList.size())) * 40);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(16,185,129));
        p.drawRoundedRect(rightX, y, barW, barH, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 6, QFont::Bold));
        p.drawText(rightX + 2, y + 2, barW - 4, barH - 4, Qt::AlignCenter, tgtList[i].left(5));
    }

    for (const auto& e : entries_) {
        int srcIdx = srcList.indexOf(e.source);
        int tgtIdx = tgtList.indexOf(e.target);
        if (srcIdx < 0 || tgtIdx < 0) continue;

        int srcBarH = qMin(srcSpacing - 4, 28);
        int tgtBarH = qMin(tgtSpacing - 4, 24);
        qreal srcWeight = 0;
        for (const auto& se : entries_) if (se.source == e.source) srcWeight += se.weight;
        int srcBarW = 20 + static_cast<int>((srcWeight / (maxWeight * srcList.size())) * 40);

        int y1 = rect.y() + 20 + srcIdx * srcSpacing + srcBarH / 2;
        int y2 = rect.y() + 20 + tgtIdx * tgtSpacing + tgtBarH / 2;
        int thickness = 1 + static_cast<int>((e.weight / maxWeight) * 5);

        QPainterPath path;
        path.moveTo(leftX + srcBarW, y1);
        int midX = (leftX + srcBarW + rightX) / 2;
        path.cubicTo(midX, y1, midX, y2, rightX, y2);

        p.setPen(QPen(QColor(e.color.red(), e.color.green(), e.color.blue(), 100), thickness));
        p.setBrush(Qt::NoBrush);
        p.drawPath(path);
    }
}

void PaperCitationSankey::drawFieldChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Fields");

    auto counts = fieldCounts();
    QStringList fields = {"ML", "NLP", "Vision", "Theory"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11), QColor(139,92,246)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(26, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(fields[i]) ? counts[fields[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 100));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 3, 55, barH, Qt::AlignRight | Qt::AlignVCenter, fields[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 60, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 63 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperCitationSankey::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Flows", QString::number(entries_.size()), QColor(59,130,246)},
        {"Weight", QString::number(static_cast<int>(totalWeight())), QColor(16,185,129)},
        {"Avg Impact", QString::number(avgImpact() * 100, 'f', 0) + "%", QColor(245,158,11)},
        {"Fields", QString::number(fieldCounts().size()), QColor(139,92,246)}
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

void PaperCitationSankey::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Citation Sankey diagram"); return; }
    infoLabel_->setText(QString("%1 flows | %2 weight | %3% impact")
        .arg(entries_.size()).arg(static_cast<int>(totalWeight())).arg(avgImpact() * 100, 0, 'f', 0));
}

void PaperCitationSankey::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        CitationSankeyEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.weight = settings_.value("weight").toDouble();
        e.flowType = settings_.value("flowType").toString();
        e.year = settings_.value("year").toInt();
        e.field = settings_.value("field").toString();
        e.impact = settings_.value("impact").toDouble();
        e.label = settings_.value("label").toString();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperCitationSankey::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("weight", entries_[i].weight);
        settings_.setValue("flowType", entries_[i].flowType);
        settings_.setValue("year", entries_[i].year);
        settings_.setValue("field", entries_[i].field);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
