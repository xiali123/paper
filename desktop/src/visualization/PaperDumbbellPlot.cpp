#include "visualization/PaperDumbbellPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDumbbellPlot::PaperDumbbellPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DumbbellPlot")
{
    setupUI();
    loadSettings();
}

void PaperDumbbellPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperDumbbellPlot::onRender);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Category-A", "Category-B", "Category-C", "Category-D"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDumbbellPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Render dumbbell plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperDumbbellPlot::addEntry(const DumbbellEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit dumbbellSelected(entry.id, entry.gap);
    update();
}

QList<DumbbellEntry> PaperDumbbellPlot::entries() const { return entries_; }

int PaperDumbbellPlot::significantCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.significant) c++;
    return c;
}

qreal PaperDumbbellPlot::maxGap() const {
    qreal mx = 0;
    for (const auto& e : entries_) if (e.gap > mx) mx = e.gap;
    return mx;
}

QMap<QString, int> PaperDumbbellPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDumbbellPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"cat-a", "cat-b", "cat-c", "cat-d"};
    QStringList groups = {"group-1", "group-2"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(6);
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    for (int i = 0; i < count; ++i) {
        DumbbellEntry e;
        e.id = entries_.size() + 1;
        e.label = text.left(6) + " db" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.group = groups[QRandomGenerator::global()->bounded(groups.size())];
        e.left = QRandomGenerator::global()->bounded(400) / 10.0;
        e.right = 20.0 + QRandomGenerator::global()->bounded(700) / 10.0;
        e.gap = qAbs(e.right - e.left);
        e.significant = e.gap > 30.0;
        e.color = colors[QRandomGenerator::global()->bounded(5)];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit dumbbellSelected(entries_.size(), entries_.last().gap);
    update();
    inputField_->clear();
}

void PaperDumbbellPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render dumbbell plot");
    update();
}

void PaperDumbbellPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render dumbbell plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Dumbbell Plot");
    int w = width(), h = height();
    drawDumbbellChart(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDumbbellPlot::drawDumbbellChart(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, rect.y() + margin, rect.x() + margin, rect.y() + margin + plotH);
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH, rect.x() + margin + plotW, rect.y() + margin + plotH);
    qreal maxVal = 90.0;
    for (const auto& e : entries_) {
        if (e.left > maxVal) maxVal = e.left;
        if (e.right > maxVal) maxVal = e.right;
    }
    int rowH = qMax(12, plotH / (entries_.size() + 1));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + margin + 20 + i * rowH;
        int lx = rect.x() + margin + static_cast<int>((e.left / maxVal) * plotW);
        int rx = rect.x() + margin + static_cast<int>((e.right / maxVal) * plotW);
        p.setPen(e.color);
        p.drawLine(lx, y, rx, y);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(lx - 4, y - 4, 8, 8);
        QColor rightColor = e.significant ? QColor(220, 38, 38) : e.color;
        p.setBrush(rightColor);
        p.drawEllipse(rx - 4, y - 4, 8, 8);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + margin, y - 8, plotW, 12, Qt::AlignLeft, e.label);
    }
}

void PaperDumbbellPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"cat-a", "cat-b", "cat-c", "cat-d"};
    QString labels[] = {"Category A", "Category B", "Category C", "Category D"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int itemH = qMin(28, (rect.height() - 50) / 5);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, labels[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " items");
    }
    int y = rect.y() + 22 + 4 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(124,58,237));
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, "Significant");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(significantCount()));
}

void PaperDumbbellPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(59,130,246)},
        {"Significant", QString::number(significantCount()), QColor(220,38,38)},
        {"Max Gap", QString::number(maxGap(), 'f', 1), QColor(217,119,6)},
        {"Categories", QString::number(categoryCounts().size()), QColor(124,58,237)}
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

void PaperDumbbellPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render dumbbell plot"); return; }
    infoLabel_->setText(QString("%1 entries | %2 significant | max gap: %3")
        .arg(entries_.size()).arg(significantCount()).arg(maxGap(), 0, 'f', 1));
}

void PaperDumbbellPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DumbbellEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.left = settings_.value("left").toDouble();
        e.right = settings_.value("right").toDouble();
        e.gap = settings_.value("gap").toDouble();
        e.significant = settings_.value("significant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDumbbellPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("left", entries_[i].left);
        settings_.setValue("right", entries_[i].right);
        settings_.setValue("gap", entries_[i].gap);
        settings_.setValue("significant", entries_[i].significant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
