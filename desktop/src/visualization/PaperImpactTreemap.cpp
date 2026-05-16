#include "visualization/PaperImpactTreemap.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <cmath>

PaperImpactTreemap::PaperImpactTreemap(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ImpactTreemap")
{
    setupUI();
    loadSettings();
}

void PaperImpactTreemap::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperImpactTreemap::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Rising", "Stable", "Declining"});
    toolbar->addWidget(filterCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperImpactTreemap::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter research area for impact treemap...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate impact treemap");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperImpactTreemap::addEntry(const TreemapEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit treemapGenerated(entry.id, entry.impact);
    update();
}

QList<TreemapEntry> PaperImpactTreemap::entries() const { return entries_; }

qreal PaperImpactTreemap::totalImpact() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.impact;
    return t;
}

int PaperImpactTreemap::topFields() const {
    int c = 0;
    for (const auto& e : entries_) if (e.impact >= 0.7) c++;
    return c;
}

QMap<QString, int> PaperImpactTreemap::trendCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.trend]++;
    return counts;
}

void PaperImpactTreemap::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList fields = {"Deep Learning", "NLP", "Computer Vision", "Robotics", "Reinforcement Learning",
                          "Generative AI", "Graph NN", "Federated Learning", "Explainability", "Optimization"};
    QStringList trends = {"rising", "stable", "declining"};
    QColor trendColors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    entries_.clear();
    int count = 6 + QRandomGenerator::global()->bounded(5);
    for (int i = 0; i < count; ++i) {
        TreemapEntry e;
        e.id = entries_.size() + 1;
        e.field = fields[i % fields.size()];
        e.paperCount = 20 + QRandomGenerator::global()->bounded(200);
        e.impact = 0.2 + QRandomGenerator::global()->bounded(80) / 100.0;
        e.size = e.paperCount * e.impact;
        int tIdx = QRandomGenerator::global()->bounded(trends.size());
        e.trend = trends[tIdx];
        e.citations = 50 + QRandomGenerator::global()->bounded(5000);
        e.topPaper = "Paper " + QString::number(1 + QRandomGenerator::global()->bounded(20));
        e.growth = -0.2 + QRandomGenerator::global()->bounded(140) / 100.0;
        e.color = trendColors[tIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit treemapGenerated(entries_.size(), totalImpact());
    update();
    inputField_->clear();
}

void PaperImpactTreemap::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate impact treemap");
    update();
}

void PaperImpactTreemap::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate impact treemap");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Impact Treemap");

    int w = width(), h = height();
    drawTreemapView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawTrendChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperImpactTreemap::drawTreemapView(QPainter& p, const QRect& rect) {
    qreal totalSize = 0;
    for (const auto& e : entries_) totalSize += e.size;
    if (totalSize == 0) return;

    int x = rect.x(), y = rect.y();
    int remainingW = rect.width(), remainingH = rect.height();

    for (int i = 0; i < entries_.size() && remainingW > 10 && remainingH > 10; ++i) {
        const auto& e = entries_[i];
        qreal fraction = e.size / totalSize;

        bool horizontal = remainingW >= remainingH;
        int slice;
        if (horizontal) {
            slice = static_cast<int>(fraction * remainingW);
            slice = qMax(slice, 20);
            slice = qMin(slice, remainingW - 10);
        } else {
            slice = static_cast<int>(fraction * remainingH);
            slice = qMax(slice, 20);
            slice = qMin(slice, remainingH - 10);
        }

        int boxW = horizontal ? slice : remainingW;
        int boxH = horizontal ? remainingH : slice;

        int alpha = 80 + static_cast<int>(e.impact * 175);
        QColor cellColor = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        p.setPen(QPen(QColor(255,255,255,100), 1));
        p.setBrush(cellColor);
        p.drawRoundedRect(x, y, boxW, boxH, 3, 3);

        p.setPen(alpha > 150 ? Qt::white : QColor(15, 23, 42));
        p.setFont(QFont("Arial", qMin(8, boxW / 8), QFont::Bold));
        p.drawText(x + 3, y + 3, boxW - 6, boxH / 2, Qt::AlignLeft | Qt::AlignTop,
                   e.field.left(boxW / 6));
        p.setFont(QFont("Arial", qMin(6, boxW / 10)));
        p.drawText(x + 3, y + boxH / 2, boxW - 6, boxH / 2 - 3, Qt::AlignLeft | Qt::AlignBottom,
                   QString::number(e.paperCount) + " papers | " + QString::number(e.citations) + " cites");

        if (horizontal) {
            x += slice;
            remainingW -= slice;
        } else {
            y += slice;
            remainingH -= slice;
        }
        totalSize -= e.size;
    }
}

void PaperImpactTreemap::drawTrendChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Trends");

    auto counts = trendCounts();
    QStringList trends = {"rising", "stable", "declining"};
    QString labels[] = {"Rising", "Stable", "Declining"};
    QColor colors[] = {QColor(16,185,129), QColor(245,158,11), QColor(239,68,68)};

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (barH + 4);
        int count = counts.contains(trends[i]) ? counts[trends[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y + barH - 3, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 3, QString::number(count));
    }
}

void PaperImpactTreemap::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Fields", QString::number(entries_.size()), QColor(59,130,246)},
        {"Top Fields", QString::number(topFields()), QColor(16,185,129)},
        {"Total Impact", QString::number(totalImpact(), 'f', 1), QColor(245,158,11)},
        {"Trends", QString::number(trendCounts().size()), QColor(139,92,246)}
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

void PaperImpactTreemap::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate impact treemap"); return; }
    infoLabel_->setText(QString("%1 fields | %2 top | %3 impact")
        .arg(entries_.size()).arg(topFields()).arg(totalImpact(), 0, 'f', 1));
}

void PaperImpactTreemap::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TreemapEntry e;
        e.id = settings_.value("id").toInt();
        e.field = settings_.value("field").toString();
        e.paperCount = settings_.value("paperCount").toInt();
        e.impact = settings_.value("impact").toDouble();
        e.size = settings_.value("size").toDouble();
        e.trend = settings_.value("trend").toString();
        e.citations = settings_.value("citations").toInt();
        e.topPaper = settings_.value("topPaper").toString();
        e.growth = settings_.value("growth").toDouble();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperImpactTreemap::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("field", entries_[i].field);
        settings_.setValue("paperCount", entries_[i].paperCount);
        settings_.setValue("impact", entries_[i].impact);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("trend", entries_[i].trend);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("topPaper", entries_[i].topPaper);
        settings_.setValue("growth", entries_[i].growth);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
