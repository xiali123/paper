#include "visualization/PaperLollipopPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperLollipopPlot::PaperLollipopPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LollipopPlot")
{
    setupUI();
    loadSettings();
}

void PaperLollipopPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperLollipopPlot::onRender);
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
    connect(clearBtn_, &QPushButton::clicked, this, &PaperLollipopPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Render lollipop plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperLollipopPlot::addEntry(const LollipopEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit stickSelected(entry.id, entry.value);
    update();
}

QList<LollipopEntry> PaperLollipopPlot::entries() const { return entries_; }

int PaperLollipopPlot::aboveCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.above) c++;
    return c;
}

qreal PaperLollipopPlot::maxValue() const {
    qreal mx = 0;
    for (const auto& e : entries_) if (e.value > mx) mx = e.value;
    return mx;
}

QMap<QString, int> PaperLollipopPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperLollipopPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"cat-a", "cat-b", "cat-c", "cat-d"};
    QStringList groups = {"group-1", "group-2"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(8);
    qreal threshold = 50.0;
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    for (int i = 0; i < count; ++i) {
        LollipopEntry e;
        e.id = entries_.size() + 1;
        e.label = text.left(6) + " L" + QString::number(i);
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.group = groups[QRandomGenerator::global()->bounded(groups.size())];
        e.value = QRandomGenerator::global()->bounded(1000) / 10.0;
        e.threshold = threshold;
        e.above = e.value >= threshold;
        int ci = categories.indexOf(e.category);
        e.color = colors[ci >= 0 ? ci : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit stickSelected(entries_.size(), entries_.last().value);
    update();
    inputField_->clear();
}

void PaperLollipopPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render lollipop plot");
    update();
}

void PaperLollipopPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render lollipop plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Lollipop Plot");
    int w = width(), h = height();
    drawLollipopChart(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperLollipopPlot::drawLollipopChart(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    int baseY = rect.y() + margin + plotH;
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, baseY, rect.x() + margin + plotW, baseY);
    p.drawLine(rect.x() + margin, rect.y() + margin, rect.x() + margin, baseY);
    qreal mx = maxValue();
    if (mx <= 0) mx = 1.0;
    qreal threshold = 50.0;
    int threshY = baseY - static_cast<int>((threshold / mx) * plotH);
    p.setPen(QPen(QColor(220, 38, 38, 100), 1, Qt::DashLine));
    p.drawLine(rect.x() + margin, threshY, rect.x() + margin + plotW, threshY);
    p.setPen(QColor(220, 38, 38, 160));
    p.setFont(QFont("Arial", 7));
    p.drawText(rect.x() + margin + 2, threshY - 3, "threshold");
    int n = entries_.size();
    if (n == 0) return;
    int step = qMax(1, plotW / (n + 1));
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int cx = rect.x() + margin + (i + 1) * step;
        int topY = baseY - static_cast<int>((e.value / mx) * plotH);
        p.setPen(QPen(e.color, 2));
        p.drawLine(cx, baseY, cx, topY);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawEllipse(cx - 5, topY - 5, 10, 10);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(cx - 20, baseY + 4, 40, 14, Qt::AlignCenter, e.label);
    }
}

void PaperLollipopPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"cat-a", "cat-b", "cat-c", "cat-d"};
    QString labels[] = {"Category A", "Category B", "Category C", "Category D"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38)};
    int itemH = qMin(28, (rect.height() - 70) / 5);
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
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " sticks");
    }
    int y = rect.y() + 22 + 4 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(124, 58, 237));
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, "Above");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(aboveCount()));
}

void PaperLollipopPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Sticks", QString::number(entries_.size()), QColor(59,130,246)},
        {"Above", QString::number(aboveCount()), QColor(220,38,38)},
        {"Max", QString::number(maxValue(), 'f', 1), QColor(217,119,6)},
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

void PaperLollipopPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render lollipop plot"); return; }
    infoLabel_->setText(QString("%1 sticks | %2 above | %3 categories")
        .arg(entries_.size()).arg(aboveCount()).arg(categoryCounts().size()));
}

void PaperLollipopPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        LollipopEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.value = settings_.value("value").toDouble();
        e.threshold = settings_.value("threshold").toDouble();
        e.above = settings_.value("above").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperLollipopPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("threshold", entries_[i].threshold);
        settings_.setValue("above", entries_[i].above);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
