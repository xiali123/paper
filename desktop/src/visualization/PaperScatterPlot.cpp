#include "visualization/PaperScatterPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperScatterPlot::PaperScatterPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ScatterPlot")
{
    setupUI();
    loadSettings();
}

void PaperScatterPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperScatterPlot::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Cluster:"));
    clusterCombo_ = new QComboBox();
    clusterCombo_->addItems({"All", "AI/ML", "Theory", "Applied"});
    toolbar->addWidget(clusterCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperScatterPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset for scatter plot...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate scatter plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperScatterPlot::addEntry(const ScatterEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit scatterGenerated(entry.id, entry.correlation);
    update();
}

QList<ScatterEntry> PaperScatterPlot::entries() const { return entries_; }

qreal PaperScatterPlot::totalCorrelation() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.correlation;
    return t;
}

int PaperScatterPlot::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.outlier) c++;
    return c;
}

QMap<QString, int> PaperScatterPlot::clusterCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.cluster]++;
    return counts;
}

void PaperScatterPlot::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList clusters = {"AI/ML", "Theory", "Applied"};
    QStringList categories = {"citations", "papers", "impact", "growth"};
    QColor clusterColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    entries_.clear();
    int cIdx = clusterCombo_->currentIndex();
    int count = 15 + QRandomGenerator::global()->bounded(20);
    for (int i = 0; i < count; ++i) {
        ScatterEntry e;
        e.id = entries_.size() + 1;
        e.label = "P" + QString::number(i);
        e.xValue = QRandomGenerator::global()->bounded(100);
        e.yValue = QRandomGenerator::global()->bounded(100);
        e.size = 3 + QRandomGenerator::global()->bounded(15);
        int clIdx = cIdx == 0 ? QRandomGenerator::global()->bounded(clusters.size()) : cIdx - 1;
        e.cluster = clusters[clIdx];
        e.paperCount = 5 + QRandomGenerator::global()->bounded(100);
        e.correlation = -0.5 + QRandomGenerator::global()->bounded(150) / 100.0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.outlier = e.xValue > 90 || e.yValue > 90 || e.xValue < 5 || e.yValue < 5;
        e.color = clusterColors[clIdx];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit scatterGenerated(entries_.size(), totalCorrelation());
    update();
    inputField_->clear();
}

void PaperScatterPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate scatter plot");
    update();
}

void PaperScatterPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate scatter plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Scatter Plot");
    int w = width(), h = height();
    drawScatterView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawClusterLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperScatterPlot::drawScatterView(QPainter& p, const QRect& rect) {
    int margin = 20;
    int plotW = rect.width() - margin * 2;
    int plotH = rect.height() - margin * 2;
    p.setPen(QPen(QColor(203, 213, 225), 1));
    for (int i = 0; i <= 4; ++i) {
        int x = rect.x() + margin + plotW * i / 4;
        p.drawLine(x, rect.y() + margin, x, rect.y() + margin + plotH);
        int y = rect.y() + margin + plotH * i / 4;
        p.drawLine(rect.x() + margin, y, rect.x() + margin + plotW, y);
    }
    for (const auto& e : entries_) {
        int px = rect.x() + margin + static_cast<int>(e.xValue / 100.0 * plotW);
        int py = rect.y() + margin + plotH - static_cast<int>(e.yValue / 100.0 * plotH);
        int alpha = e.outlier ? 200 : 140;
        QColor fill = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        p.setPen(e.outlier ? QPen(Qt::red, 1) : Qt::NoPen);
        p.setBrush(fill);
        p.drawEllipse(px - e.size / 2, py - e.size / 2, e.size, e.size);
    }
}

void PaperScatterPlot::drawClusterLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Clusters");
    auto counts = clusterCounts();
    QStringList clusters = {"AI/ML", "Theory", "Applied"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
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
                   QString::number(count) + " points");
    }
}

void PaperScatterPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Points", QString::number(entries_.size()), QColor(59,130,246)},
        {"Outliers", QString::number(outlierCount()), QColor(239,68,68)},
        {"Correlation", QString::number(entries_.isEmpty() ? 0 : totalCorrelation() / entries_.size(), 'f', 2), QColor(245,158,11)},
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

void PaperScatterPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate scatter plot"); return; }
    infoLabel_->setText(QString("%1 points | %2 outliers | %3 clusters")
        .arg(entries_.size()).arg(outlierCount()).arg(clusterCounts().size()));
}

void PaperScatterPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ScatterEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.xValue = settings_.value("xValue").toDouble();
        e.yValue = settings_.value("yValue").toDouble();
        e.size = settings_.value("size").toInt();
        e.cluster = settings_.value("cluster").toString();
        e.paperCount = settings_.value("paperCount").toInt();
        e.correlation = settings_.value("correlation").toDouble();
        e.category = settings_.value("category").toString();
        e.outlier = settings_.value("outlier").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperScatterPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("xValue", entries_[i].xValue);
        settings_.setValue("yValue", entries_[i].yValue);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("cluster", entries_[i].cluster);
        settings_.setValue("paperCount", entries_[i].paperCount);
        settings_.setValue("correlation", entries_[i].correlation);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("outlier", entries_[i].outlier);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
