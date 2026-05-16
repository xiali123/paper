#include "visualization/PaperViolinPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperViolinPlot::PaperViolinPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ViolinPlot")
{
    setupUI();
    loadSettings();
}

void PaperViolinPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperViolinPlot::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Group A", "Group B", "Group C"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperViolinPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate violin plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperViolinPlot::addEntry(const ViolinEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit violinGenerated(entry.id, entry.median);
    update();
}

QList<ViolinEntry> PaperViolinPlot::entries() const { return entries_; }

int PaperViolinPlot::skewedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.skewed) c++;
    return c;
}

qreal PaperViolinPlot::avgMedian() const {
    if (entries_.isEmpty()) return 0;
    qreal sum = 0;
    for (const auto& e : entries_) sum += e.median;
    return sum / entries_.size();
}

QMap<QString, int> PaperViolinPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperViolinPlot::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"groupA", "groupB", "groupC"};
    QStringList labels = {"Metric A", "Metric B", "Metric C", "Metric D", "Metric E"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        ViolinEntry e;
        e.id = entries_.size() + 1;
        e.label = labels[i % labels.size()];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.median = 20 + QRandomGenerator::global()->bounded(60);
        e.q1 = e.median - 5 - QRandomGenerator::global()->bounded(15);
        e.q3 = e.median + 5 + QRandomGenerator::global()->bounded(15);
        e.min = e.q1 - QRandomGenerator::global()->bounded(20);
        e.max = e.q3 + QRandomGenerator::global()->bounded(20);
        e.density = 0.5 + QRandomGenerator::global()->bounded(50) / 100.0;
        e.skewed = qAbs(e.median - (e.min + e.max) / 2.0) > 10;
        QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
        int ci = categories.indexOf(e.category);
        e.color = catColors[ci >= 0 ? ci : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit violinGenerated(entries_.size(), avgMedian());
    update();
    inputField_->clear();
}

void PaperViolinPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate violin plot");
    update();
}

void PaperViolinPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate violin plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Violin Plot");
    int w = width(), h = height();
    drawViolinView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperViolinPlot::drawViolinView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int colW = (rect.width() - 10) / qMax(n, 1);
    qreal globalMax = 1;
    for (const auto& e : entries_) globalMax = qMax(globalMax, e.max);
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int cx = rect.x() + 5 + i * colW + colW / 2;
        int hMin = rect.y() + rect.height() - static_cast<int>((e.min / globalMax) * (rect.height() - 20));
        int hMax = rect.y() + rect.height() - static_cast<int>((e.max / globalMax) * (rect.height() - 20));
        int hQ1 = rect.y() + rect.height() - static_cast<int>((e.q1 / globalMax) * (rect.height() - 20));
        int hQ3 = rect.y() + rect.height() - static_cast<int>((e.q3 / globalMax) * (rect.height() - 20));
        int hMed = rect.y() + rect.height() - static_cast<int>((e.median / globalMax) * (rect.height() - 20));
        int halfW = static_cast<int>(e.density * colW * 0.4);
        // Violin shape
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(160));
        p.drawEllipse(cx - halfW, hMax, halfW * 2, hMin - hMax);
        // IQR box
        p.setBrush(e.color);
        p.drawRect(cx - 4, hQ3, 8, hQ1 - hQ3);
        // Median line
        p.setPen(QColor(15, 23, 42));
        p.drawLine(cx - 6, hMed, cx + 6, hMed);
        // Label
        p.setFont(QFont("Arial", 6));
        p.drawText(cx - colW / 2, rect.y() + rect.height() + 2, colW, 12, Qt::AlignCenter, e.label.left(6));
    }
}

void PaperViolinPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Groups");
    auto counts = categoryCounts();
    QStringList categories = {"groupA", "groupB", "groupC"};
    QString labels[] = {"Group A", "Group B", "Group C"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 30) / 3);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " items");
    }
}

void PaperViolinPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Skewed", QString::number(skewedCount()), QColor(239,68,68)},
        {"Avg Median", QString::number(avgMedian(), 'f', 1), QColor(245,158,11)},
        {"Groups", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperViolinPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate violin plot"); return; }
    infoLabel_->setText(QString("%1 items | %2 skewed | %3 avg med")
        .arg(entries_.size()).arg(skewedCount()).arg(avgMedian(), 0, 'f', 1));
}

void PaperViolinPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ViolinEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.median = settings_.value("median").toDouble();
        e.q1 = settings_.value("q1").toDouble();
        e.q3 = settings_.value("q3").toDouble();
        e.min = settings_.value("min").toDouble();
        e.max = settings_.value("max").toDouble();
        e.density = settings_.value("density").toDouble();
        e.skewed = settings_.value("skewed").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperViolinPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("median", entries_[i].median);
        settings_.setValue("q1", entries_[i].q1);
        settings_.setValue("q3", entries_[i].q3);
        settings_.setValue("min", entries_[i].min);
        settings_.setValue("max", entries_[i].max);
        settings_.setValue("density", entries_[i].density);
        settings_.setValue("skewed", entries_[i].skewed);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
