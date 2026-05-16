#include "visualization/PaperTreemapChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperTreemapChart::PaperTreemapChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TreemapChart")
{
    setupUI();
    loadSettings();
}

void PaperTreemapChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperTreemapChart::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Topics", "Authors", "Journals"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTreemapChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset for treemap...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate treemap chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperTreemapChart::addEntry(const TreemapEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit treemapGenerated(entry.id, entry.value);
    update();
}

QList<TreemapEntry> PaperTreemapChart::entries() const { return entries_; }

qreal PaperTreemapChart::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

int PaperTreemapChart::leafCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.leaf) c++;
    return c;
}

QMap<QString, int> PaperTreemapChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTreemapChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"topics", "authors", "journals"};
    QStringList names = {"Machine Learning", "NLP", "Computer Vision", "Robotics", "Security", "Theory", "Graphics", "HCI"};
    QStringList parents = {"root", "AI", "Systems", "Theory"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 8 + QRandomGenerator::global()->bounded(8);
    for (int i = 0; i < count; ++i) {
        TreemapEntry e;
        e.id = entries_.size() + 1;
        e.name = names[i % names.size()];
        e.value = 10 + QRandomGenerator::global()->bounded(100);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.depth = QRandomGenerator::global()->bounded(3);
        e.parent = parents[QRandomGenerator::global()->bounded(parents.size())];
        e.leaf = e.depth == 2 || QRandomGenerator::global()->bounded(2) == 0;
        e.percentage = 0;
        entries_.append(e);
    }
    qreal total = totalValue();
    for (auto& e : entries_) {
        e.percentage = e.value / qMax(total, 1.0);
        e.area = e.percentage;
    }
    QColor catColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    for (auto& e : entries_) {
        int ci = categories.indexOf(e.category);
        e.color = catColors[ci >= 0 ? ci : 0];
    }
    saveSettings();
    updateInfo();
    emit treemapGenerated(entries_.size(), totalValue());
    update();
    inputField_->clear();
}

void PaperTreemapChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate treemap chart");
    update();
}

void PaperTreemapChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate treemap chart");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Treemap Chart");
    int w = width(), h = height();
    drawTreemapView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTreemapChart::drawTreemapView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int cols = qMax(1, static_cast<int>(qSqrt(n)));
    int rows = (n + cols - 1) / cols;
    int cellW = (rect.width() - 10) / cols;
    int cellH = (rect.height() - 10) / rows;
    qreal maxVal = 1;
    for (const auto& e : entries_) maxVal = qMax(maxVal, e.value);
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int col = i % cols;
        int row = i / cols;
        qreal ratio = e.value / maxVal;
        int padX = static_cast<int>((1 - ratio) * cellW * 0.3);
        int padY = static_cast<int>((1 - ratio) * cellH * 0.3);
        int x = rect.x() + 5 + col * cellW + padX;
        int y = rect.y() + 5 + row * cellH + padY;
        int w = cellW - padX * 2 - 2;
        int h = cellH - padY * 2 - 2;
        p.setPen(Qt::NoPen);
        p.setBrush(e.color.lighter(100 + static_cast<int>(ratio * 80)));
        p.drawRoundedRect(x, y, w, h, 4, 4);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", qMax(6, qMin(9, w / 10))));
        p.drawText(x + 4, y + 4, w - 8, 14, Qt::AlignVCenter, e.name.left(10));
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 6));
        p.drawText(x + 4, y + 18, w - 8, 10, Qt::AlignVCenter, QString::number(e.value));
    }
}

void PaperTreemapChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"topics", "authors", "journals"};
    QString labels[] = {"Topics", "Authors", "Journals"};
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
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " items");
    }
}

void PaperTreemapChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Items", QString::number(entries_.size()), QColor(59,130,246)},
        {"Leaves", QString::number(leafCount()), QColor(16,185,129)},
        {"Total Value", QString::number(totalValue(), 'f', 0), QColor(245,158,11)},
        {"Categories", QString::number(categoryCounts().size()), QColor(139,92,246)}
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

void PaperTreemapChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate treemap chart"); return; }
    infoLabel_->setText(QString("%1 items | %2 leaves | %3 total")
        .arg(entries_.size()).arg(leafCount()).arg(totalValue(), 0, 'f', 0));
}

void PaperTreemapChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TreemapEntry e;
        e.id = settings_.value("id").toInt();
        e.name = settings_.value("name").toString();
        e.value = settings_.value("value").toDouble();
        e.area = settings_.value("area").toDouble();
        e.category = settings_.value("category").toString();
        e.depth = settings_.value("depth").toInt();
        e.parent = settings_.value("parent").toString();
        e.percentage = settings_.value("percentage").toDouble();
        e.leaf = settings_.value("leaf").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTreemapChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("name", entries_[i].name);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("area", entries_[i].area);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("depth", entries_[i].depth);
        settings_.setValue("parent", entries_[i].parent);
        settings_.setValue("percentage", entries_[i].percentage);
        settings_.setValue("leaf", entries_[i].leaf);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
