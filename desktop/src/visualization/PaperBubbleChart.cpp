#include "visualization/PaperBubbleChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperBubbleChart::PaperBubbleChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "BubbleChart")
{
    setupUI();
    loadSettings();
}

void PaperBubbleChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperBubbleChart::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Cluster:"));
    clusterCombo_ = new QComboBox();
    clusterCombo_->addItems({"All", "AI/ML", "Theory", "Applied", "Interdisciplinary"});
    toolbar->addWidget(clusterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperBubbleChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter research area for bubble chart...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate bubble chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperBubbleChart::addEntry(const BubbleEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit bubbleGenerated(entry.id, entry.impact);
    update();
}

QList<BubbleEntry> PaperBubbleChart::entries() const { return entries_; }

qreal PaperBubbleChart::totalImpact() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.impact;
    return t;
}

int PaperBubbleChart::highlightedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlighted) c++;
    return c;
}

QMap<QString, int> PaperBubbleChart::clusterCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.cluster]++;
    return counts;
}

void PaperBubbleChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList fields = {"Deep Learning", "NLP", "Computer Vision", "Robotics", "Reinforcement Learning",
                          "Graph Theory", "Optimization", "Bioinformatics"};
    QStringList clusters = {"AI/ML", "Theory", "Applied", "Interdisciplinary"};
    QColor clusterColors[] = {QColor(59,130,246), QColor(139,92,246), QColor(16,185,129), QColor(245,158,11)};

    entries_.clear();
    int count = 6 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        BubbleEntry e;
        e.id = entries_.size() + 1;
        e.field = fields[i % fields.size()];
        e.xValue = QRandomGenerator::global()->bounded(100);
        e.yValue = QRandomGenerator::global()->bounded(100);
        e.paperCount = 10 + QRandomGenerator::global()->bounded(200);
        e.bubbleSize = e.paperCount * 0.3;
        e.impact = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        int cIdx = QRandomGenerator::global()->bounded(clusters.size());
        e.cluster = clusters[cIdx];
        e.rank = i + 1;
        e.highlighted = e.impact >= 0.8;
        e.color = clusterColors[cIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit bubbleGenerated(entries_.size(), totalImpact());
    update();
    inputField_->clear();
}

void PaperBubbleChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate bubble chart");
    update();
}

void PaperBubbleChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate bubble chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Bubble Chart");

    int w = width(), h = height();
    drawBubbleView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawClusterLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperBubbleChart::drawBubbleView(QPainter& p, const QRect& rect) {
    int maxPapers = 1;
    for (const auto& e : entries_) maxPapers = qMax(maxPapers, e.paperCount);

    for (const auto& e : entries_) {
        qreal normX = e.xValue / 100.0;
        qreal normY = e.yValue / 100.0;
        int cx = rect.x() + static_cast<int>(normX * rect.width());
        int cy = rect.y() + static_cast<int>(normY * rect.height());
        int radius = qMax(8, static_cast<int>((static_cast<qreal>(e.paperCount) / maxPapers) * 30));

        int alpha = e.highlighted ? 200 : 120;
        QColor fillColor = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        p.setPen(e.highlighted ? QPen(e.color, 2) : QPen(e.color.lighter(150), 1));
        p.setBrush(fillColor);
        p.drawEllipse(cx - radius, cy - radius, radius * 2, radius * 2);

        if (radius > 12) {
            p.setPen(alpha > 150 ? Qt::white : QColor(15, 23, 42));
            p.setFont(QFont("Arial", qMin(7, radius / 3), QFont::Bold));
            p.drawText(cx - radius, cy - 4, radius * 2, 10, Qt::AlignCenter,
                       e.field.left(radius / 4));
        }
    }
}

void PaperBubbleChart::drawClusterLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Clusters");

    auto counts = clusterCounts();
    QStringList clusters = {"AI/ML", "Theory", "Applied", "Interdisciplinary"};
    QColor colors[] = {QColor(59,130,246), QColor(139,92,246), QColor(16,185,129), QColor(245,158,11)};

    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(clusters[i]) ? counts[clusters[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, clusters[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " fields");
    }
}

void PaperBubbleChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Bubbles", QString::number(entries_.size()), QColor(59,130,246)},
        {"Highlighted", QString::number(highlightedCount()), QColor(16,185,129)},
        {"Total Impact", QString::number(totalImpact(), 'f', 1), QColor(245,158,11)},
        {"Clusters", QString::number(clusterCounts().size()), QColor(139,92,246)}
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

void PaperBubbleChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate bubble chart"); return; }
    infoLabel_->setText(QString("%1 bubbles | %2 highlighted | %3 impact")
        .arg(entries_.size()).arg(highlightedCount()).arg(totalImpact(), 0, 'f', 1));
}

void PaperBubbleChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        BubbleEntry e;
        e.id = settings_.value("id").toInt();
        e.field = settings_.value("field").toString();
        e.xValue = settings_.value("xValue").toDouble();
        e.yValue = settings_.value("yValue").toDouble();
        e.bubbleSize = settings_.value("bubbleSize").toDouble();
        e.paperCount = settings_.value("paperCount").toInt();
        e.impact = settings_.value("impact").toDouble();
        e.cluster = settings_.value("cluster").toString();
        e.rank = settings_.value("rank").toInt();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperBubbleChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("field", entries_[i].field);
        settings_.setValue("xValue", entries_[i].xValue);
        settings_.setValue("yValue", entries_[i].yValue);
        settings_.setValue("bubbleSize", entries_[i].bubbleSize);
        settings_.setValue("paperCount", entries_[i].paperCount);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("cluster", entries_[i].cluster);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
