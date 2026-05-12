#include "visualization/PaperScatterPlot3.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperScatterPlot3::PaperScatterPlot3(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ScatterPlot3")
{
    setupUI();
    loadSettings();
}

void PaperScatterPlot3::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperScatterPlot3::onRender);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Research", "Review", "Survey", "Case Study"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset for scatter plot...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperScatterPlot3::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Render scatter plot");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperScatterPlot3::addEntry(const Scatter3Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit pointClicked(entry.id, entry.x);
    update();
}

QList<Scatter3Entry> PaperScatterPlot3::entries() const { return entries_; }

int PaperScatterPlot3::selectedCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.selected) c++;
    return c;
}

qreal PaperScatterPlot3::maxX() const {
    qreal m = 0;
    for (const auto& e : entries_) if (e.x > m) m = e.x;
    return m;
}

QMap<QString, int> PaperScatterPlot3::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperScatterPlot3::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    QStringList categories = {"Research", "Review", "Survey", "Case Study", "Meta"};
    QStringList axes = {"impact", "citations", "references", "downloads"};
    QColor catColors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    entries_.clear();
    int cIdx = categoryCombo_->currentIndex();
    int count = 5 + QRandomGenerator::global()->bounded(8);
    for (int i = 0; i < count; ++i) {
        Scatter3Entry e;
        e.id = entries_.size() + 1;
        e.label = text.left(6) + " s" + QString::number(i);
        int ci = cIdx == 0 ? QRandomGenerator::global()->bounded(categories.size()) : cIdx - 1;
        e.category = categories[ci];
        e.axis = axes[QRandomGenerator::global()->bounded(axes.size())];
        e.x = QRandomGenerator::global()->bounded(1000) / 10.0;
        e.y = QRandomGenerator::global()->bounded(1000) / 10.0;
        e.size = 4 + QRandomGenerator::global()->bounded(14);
        e.selected = QRandomGenerator::global()->bounded(5) == 0;
        e.color = catColors[ci];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit pointClicked(entries_.size(), maxX());
    update();
    inputField_->clear();
}

void PaperScatterPlot3::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render scatter plot");
    update();
}

void PaperScatterPlot3::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render scatter plot");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Scatter Plot 3D");
    int w = width(), h = height();
    drawScatterView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperScatterPlot3::drawScatterView(QPainter& p, const QRect& rect) {
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
        int px = rect.x() + margin + static_cast<int>(e.x / 100.0 * plotW);
        int py = rect.y() + margin + plotH - static_cast<int>(e.y / 100.0 * plotH);
        int sz = static_cast<int>(e.size);
        int alpha = e.selected ? 220 : 150;
        QColor fill = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);
        p.setPen(e.selected ? QPen(Qt::black, 2) : Qt::NoPen);
        p.setBrush(fill);
        p.drawEllipse(px - sz / 2, py - sz / 2, sz, sz);
    }
}

void PaperScatterPlot3::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    auto counts = categoryCounts();
    QStringList categories = {"Research", "Review", "Survey", "Case Study", "Meta"};
    QColor colors[] = {QColor(59,130,246), QColor(22,163,74), QColor(217,119,6), QColor(220,38,38), QColor(124,58,237)};
    int itemH = qMin(28, (rect.height() - 30) / 5);
    for (int i = 0; i < 5; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawEllipse(rect.x() + 5, y + 4, 14, 14);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18, Qt::AlignVCenter, categories[i]);
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " points");
    }
}

void PaperScatterPlot3::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Points", QString::number(entries_.size()), QColor(59,130,246)},
        {"Selected", QString::number(selectedCount()), QColor(124,58,237)},
        {"Max X", QString::number(maxX(), 'f', 1), QColor(22,163,74)},
        {"Categories", QString::number(categoryCounts().size()), QColor(217,119,6)}
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

void PaperScatterPlot3::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Render scatter plot"); return; }
    infoLabel_->setText(QString("%1 points | %2 selected | %3 categories")
        .arg(entries_.size()).arg(selectedCount()).arg(categoryCounts().size()));
}

void PaperScatterPlot3::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        Scatter3Entry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.axis = settings_.value("axis").toString();
        e.x = settings_.value("x").toDouble();
        e.y = settings_.value("y").toDouble();
        e.size = settings_.value("size").toDouble();
        e.selected = settings_.value("selected").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperScatterPlot3::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("axis", entries_[i].axis);
        settings_.setValue("x", entries_[i].x);
        settings_.setValue("y", entries_[i].y);
        settings_.setValue("size", entries_[i].size);
        settings_.setValue("selected", entries_[i].selected);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
