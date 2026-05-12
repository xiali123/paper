#include "visualization/PaperStripPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperStripPlot::PaperStripPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "StripPlot")
{
    setupUI();
    loadSettings();
}

void PaperStripPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperStripPlot::onRender);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Cat-A", "Cat-B", "Cat-C", "Cat-D"});
    toolbar->addWidget(categoryCombo_, 1);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStripPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);
    infoLabel_ = new QLabel("Render strip plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperStripPlot::addEntry(const StripEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit stripSelected(entry.id, entry.position);
    update();
}

QList<StripEntry> PaperStripPlot::entries() const { return entries_; }

int PaperStripPlot::highlightCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.highlight) c++;
    return c;
}

qreal PaperStripPlot::maxPosition() const {
    qreal mx = 0;
    for (const auto& e : entries_) if (e.position > mx) mx = e.position;
    return mx;
}

QMap<QString, int> PaperStripPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperStripPlot::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList groups = {"group-a", "group-b", "group-c"};
    QStringList categories = {"cat-a", "cat-b", "cat-c", "cat-d"};
    int cIdx = categoryCombo_->currentIndex();
    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(8);
    QColor palette[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    for (int i = 0; i < count; ++i) {
        StripEntry e;
        e.id = entries_.size() + 1;
        e.label = text.left(6) + " s" + QString::number(i);
        e.group = groups[QRandomGenerator::global()->bounded(groups.size())];
        e.category = cIdx == 0 ? categories[QRandomGenerator::global()->bounded(categories.size())] : categories[cIdx - 1];
        e.position = QRandomGenerator::global()->bounded(1000) / 10.0;
        e.width = 2 + QRandomGenerator::global()->bounded(6);
        e.highlight = e.position > 90 || e.position < 10;
        int ci = categories.indexOf(e.category);
        e.color = e.highlight ? QColor(220,38,38) : palette[ci >= 0 ? ci % 5 : 0];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit stripSelected(entries_.size(), entries_.last().position);
    update();
    inputField_->clear();
}

void PaperStripPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render strip plot");
    update();
}

void PaperStripPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render strip plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Strip Plot");
    int w = width(), h = height();
    drawStripChart(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperStripPlot::drawStripChart(QPainter& p, const QRect& rect) {
    int margin = 10;
    int plotW = rect.width() - 2 * margin;
    int plotH = rect.height() - 2 * margin;
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.x() + margin, rect.y() + margin, rect.x() + margin, rect.y() + margin + plotH);
    p.drawLine(rect.x() + margin, rect.y() + margin + plotH, rect.x() + margin + plotW, rect.y() + margin + plotH);
    qreal mx = maxPosition();
    if (mx <= 0) mx = 100.0;
    int stripH = qMax(4, qMin(18, plotH / (entries_.size() + 1)));
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        int yPos = rect.y() + margin + static_cast<int>((i + 0.5) / entries_.size() * plotH);
        int xPos = rect.x() + margin + static_cast<int>((e.position / mx) * plotW);
        int barW = static_cast<int>((e.width / mx) * plotW);
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(xPos, yPos - stripH / 2, qMax(barW, 4), stripH, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + margin, yPos - stripH / 2, qMax(barW, 4), stripH, Qt::AlignRight | Qt::AlignVCenter, "");
    }
}

void PaperStripPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"cat-a", "cat-b", "cat-c", "cat-d"};
    QString labels[] = {"Cat A", "Cat B", "Cat C", "Cat D"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(124,58,237)};
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
                   Qt::AlignVCenter | Qt::AlignRight, QString::number(count) + " strips");
    }
    int y = rect.y() + 22 + 4 * (itemH + 4);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(220,38,38));
    p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, "Highlighted");
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
               Qt::AlignVCenter | Qt::AlignRight, QString::number(highlightCount()));
}

void PaperStripPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Strips", QString::number(entries_.size()), QColor(59,130,246)},
        {"Highlighted", QString::number(highlightCount()), QColor(220,38,38)},
        {"Max Position", QString::number(maxPosition(), 'f', 1), QColor(217,119,6)},
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

void PaperStripPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render strip plot"); return; }
    infoLabel_->setText(QString("%1 strips | %2 highlighted | max %3")
        .arg(entries_.size()).arg(highlightCount()).arg(maxPosition(), 0, 'f', 1));
}

void PaperStripPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        StripEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.group = settings_.value("group").toString();
        e.position = settings_.value("position").toDouble();
        e.width = settings_.value("width").toDouble();
        e.highlight = settings_.value("highlight").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperStripPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("group", entries_[i].group);
        settings_.setValue("position", entries_[i].position);
        settings_.setValue("width", entries_[i].width);
        settings_.setValue("highlight", entries_[i].highlight);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
