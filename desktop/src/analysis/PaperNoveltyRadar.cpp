#include "analysis/PaperNoveltyRadar.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperNoveltyRadar::PaperNoveltyRadar(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoveltyRadar")
{
    setupUI();
    loadSettings();
}

void PaperNoveltyRadar::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    scanBtn_ = new QPushButton("Scan");
    scanBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(scanBtn_, &QPushButton::clicked, this, &PaperNoveltyRadar::onScan);
    toolbar->addWidget(scanBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Method", "Theory", "Application", "Dataset"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper or topic...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoveltyRadar::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Scan for novelty signals");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(580, 480);
}

void PaperNoveltyRadar::addEntry(const NoveltyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit noveltyDetected(entry.id, entry.score);
    update();
}

QList<NoveltyEntry> PaperNoveltyRadar::entries() const { return entries_; }

int PaperNoveltyRadar::breakthroughCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.breakthrough) c++;
    return c;
}

qreal PaperNoveltyRadar::avgScore() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperNoveltyRadar::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperNoveltyRadar::onScan() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Method", "Theory", "Application", "Dataset"};
    QStringList dimensions = {"novelty", "impact", "reproducibility", "scalability", "generality"};
    int cIdx = categoryCombo_->currentIndex();
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        NoveltyEntry e;
        e.id = entries_.size() + 1;
        e.paper = text.left(10) + " paper" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.dimension = dimensions[QRandomGenerator::global()->bounded(dimensions.size())];
        e.score = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        e.citations = QRandomGenerator::global()->bounded(200);
        e.breakthrough = e.score >= 0.75 && e.citations >= 50;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNoveltyRadar::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Scan for novelty signals");
    update();
}

void PaperNoveltyRadar::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Scan for novelty signals");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Novelty Radar");
    int w = width(), h = height();
    drawRadarChart(p, QRect(20, 50, w / 2 - 20, h / 2 - 30));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(20, h / 2 + 30, w - 40, h / 2 - 50));
}

void PaperNoveltyRadar::drawRadarChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Radar");
    QStringList dims = {"novelty", "impact", "reproducibility", "scalability", "generality"};
    int n = dims.size();
    QPointF center(rect.x() + rect.width() / 2, rect.y() + 30 + (rect.height() - 30) / 2);
    qreal radius = qMin(rect.width(), rect.height() - 30) / 2 - 20;
    for (int ring = 1; ring <= 4; ++ring) {
        qreal r = radius * ring / 4;
        p.setPen(QPen(QColor(226, 232, 240), 1));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(center, r, r);
    }
    QMap<QString, qreal> dimAvg;
    QMap<QString, int> dimCount;
    for (const auto& e : entries_) {
        dimAvg[e.dimension] += e.score;
        dimCount[e.dimension]++;
    }
    for (int i = 0; i < n; ++i) {
        qreal angle = 2 * M_PI * i / n - M_PI / 2;
        QPointF edge(center.x() + radius * qCos(angle), center.y() + radius * qSin(angle));
        p.setPen(QPen(QColor(203, 213, 225), 1));
        p.drawLine(center, edge);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        QPointF labelPt(center.x() + (radius + 14) * qCos(angle) - 20, center.y() + (radius + 14) * qSin(angle) - 5);
        p.drawText(labelPt, dims[i].left(1).toUpper() + dims[i].mid(1, 4));
    }
    QPainterPath path;
    for (int i = 0; i < n; ++i) {
        qreal angle = 2 * M_PI * i / n - M_PI / 2;
        qreal val = dimCount.contains(dims[i]) ? dimAvg[dims[i]] / dimCount[dims[i]] : 0;
        qreal r = radius * val;
        QPointF pt(center.x() + r * qCos(angle), center.y() + r * qSin(angle));
        if (i == 0) path.moveTo(pt);
        else path.lineTo(pt);
    }
    path.closeSubpath();
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(59, 130, 246, 60));
    p.drawPath(path);
    p.setPen(QPen(QColor(59, 130, 246), 2));
    p.setBrush(Qt::NoBrush);
    p.drawPath(path);
    for (int i = 0; i < n; ++i) {
        qreal angle = 2 * M_PI * i / n - M_PI / 2;
        qreal val = dimCount.contains(dims[i]) ? dimAvg[dims[i]] / dimCount[dims[i]] : 0;
        qreal r = radius * val;
        QPointF pt(center.x() + r * qCos(angle), center.y() + r * qSin(angle));
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(59, 130, 246));
        p.drawEllipse(pt, 4, 4);
    }
}

void PaperNoveltyRadar::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList cats = {"Method", "Theory", "Application", "Dataset"};
    QString labels[] = {"Method", "Theory", "Applic.", "Dataset"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);
    int barH = qMin(24, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (barH + 3);
        int count = counts.contains(cats[i]) ? counts[cats[i]] : 0;
        int barW = static_cast<int>((static_cast<qreal>(count) / maxVal) * (rect.width() - 110));
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 65, barH, Qt::AlignRight | Qt::AlignVCenter, labels[i]);
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 70, y, barW, barH - 2, 3, 3);
        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 73 + barW, y + barH - 2, QString::number(count));
    }
}

void PaperNoveltyRadar::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Breakthroughs", QString::number(breakthroughCount()), QColor(22,163,74)},
        {"Avg Score", QString::number(avgScore() * 100, 'f', 0) + "%", QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
    };
    int boxW = (rect.width() - 30) / 4;
    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, rect.y(), boxW, rect.height(), 6, 6);
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(x + 10, rect.y() + 10, boxW - 20, 30, Qt::AlignVCenter, stats[i].value);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, rect.y() + 38, boxW - 20, 20, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperNoveltyRadar::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Scan for novelty signals"); return; }
    infoLabel_->setText(QString("%1 entries | %2 breakthroughs | %3% avg score")
        .arg(entries_.size()).arg(breakthroughCount()).arg(avgScore() * 100, 0, 'f', 0));
}

void PaperNoveltyRadar::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoveltyEntry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.dimension = settings_.value("dimension").toString();
        e.score = settings_.value("score").toDouble();
        e.citations = settings_.value("citations").toInt();
        e.breakthrough = settings_.value("breakthrough").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNoveltyRadar::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("dimension", entries_[i].dimension);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("breakthrough", entries_[i].breakthrough);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
