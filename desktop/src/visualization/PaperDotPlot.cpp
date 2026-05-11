#include "visualization/PaperDotPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDotPlot::PaperDotPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DotPlot")
{
    setupUI();
    loadSettings();
}

void PaperDotPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperDotPlot::onGenerate);
    toolbar->addWidget(generateBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Group-A", "Group-B", "Group-C"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDotPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Generate dot plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperDotPlot::addEntry(const DotEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dotPlotted(entry.id, entry.x);
    update();
}

QList<DotEntry> PaperDotPlot::entries() const { return entries_; }

int PaperDotPlot::outlierCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.outlier) c++;
    return c;
}

QMap<QString, int> PaperDotPlot::groupCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.group]++;
    return counts;
}

QMap<QString, int> PaperDotPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDotPlot::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList groups = {"group-a", "group-b", "group-c"};
    QStringList categories = {"type-1", "type-2", "type-3"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 15 + QRandomGenerator::global()->bounded(20);
    for (int i = 0; i < count; ++i) {
        DotEntry e;
        e.id = entries_.size() + 1;
        e.label = text.left(6) + " d" + QString::number(i);
        e.group = cIdx == 0 ? groups[QRandomGenerator::global()->bounded(groups.size())] : groups[cIdx - 1];
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.x = QRandomGenerator::global()->bounded(1000) / 10.0;
        e.y = QRandomGenerator::global()->bounded(1000) / 10.0;
        e.size = 4 + QRandomGenerator::global()->bounded(12);
        e.outlier = e.x > 85 || e.y > 85 || e.x < 10 || e.y < 10;
        QColor groupColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
        int gi = groups.indexOf(e.group);
        e.color = e.outlier ? QColor(239,68,68) : groupColors[gi >= 0 ? gi : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit dotPlotted(entries_.size(), entries_.last().x);
    update();
    inputField_->clear();
}

void PaperDotPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate dot plot");
    update();
}

void PaperDotPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate dot plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Dot Plot");
    int w = width(), h = height();
    drawDotView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawGroupLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDotPlot::drawDotView(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    // axes
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, rect.y() + margin, rect.x() + margin, rect.y() + margin + plotH);
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH, rect.x() + margin + plotW, rect.y() + margin + plotH);
    // dots
    for (const auto& e : entries_) {
        int dx = rect.x() + margin + static_cast<int>((e.x / 100.0) * plotW);
        int dy = rect.y() + margin + plotH - static_cast<int>((e.y / 100.0) * plotH);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(dx - e.size / 2, dy - e.size / 2, e.size, e.size);
    }
}

void PaperDotPlot::drawGroupLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Groups");
    auto counts = groupCounts();
    QStringList groups = {"group-a", "group-b", "group-c"};
    QString labels[] = {"Group A", "Group B", "Group C"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11)};
    int itemH = qMin(28, (rect.height() - 50) / 4);
    for (int i = 0; i < 3; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(groups[i]) ? counts[groups[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " dots");
    }
    // outlier legend
    int y = rect.y() + 22 + 3 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(239,68,68));
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, "Outliers");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(outlierCount()));
}

void PaperDotPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Dots", QString::number(entries_.size()), QColor(59,130,246)},
        {"Outliers", QString::number(outlierCount()), QColor(239,68,68)},
        {"Groups", QString::number(groupCounts().size()), QColor(245,158,11)},
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

void PaperDotPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate dot plot"); return; }
    infoLabel_->setText(QString("%1 dots | %2 outliers | %3 groups")
        .arg(entries_.size()).arg(outlierCount()).arg(groupCounts().size()));
}

void PaperDotPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DotEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.group = settings_.value("group").toString();
        e.category = settings_.value("category").toString();
        e.x = settings_.value("x").toDouble();
        e.y = settings_.value("y").toDouble();
        e.size = settings_.value("size").toDouble();
        e.outlier = settings_.value("outlier").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDotPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("x", entries_[i].x);
        settings_.setValue("y", entries_[i].y);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("outlier", entries_[i].outlier);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
