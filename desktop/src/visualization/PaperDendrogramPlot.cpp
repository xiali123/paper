#include "visualization/PaperDendrogramPlot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDendrogramPlot::PaperDendrogramPlot(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DendrogramPlot")
{
    setupUI();
    loadSettings();
}

void PaperDendrogramPlot::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();
    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperDendrogramPlot::onRender);
    toolbar->addWidget(renderBtn_);
    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Hierarchical", "Agglomerative", "Divisive", "Spectral", "BIRCH"});
    toolbar->addWidget(categoryCombo_, 1);
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter cluster label...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_);
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDendrogramPlot::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);
    infoLabel_ = new QLabel("Entries: 0 | Roots: 0 | Max Dist: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);
    setMinimumSize(600, 500);
}

void PaperDendrogramPlot::addEntry(const DendrogramEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit clusterSelected(entry.id, entry.distance);
    update();
}

QList<DendrogramEntry> PaperDendrogramPlot::entries() const { return entries_; }

int PaperDendrogramPlot::rootCount() const {
    int c = 0;
    for (const auto& e : entries_) if (e.root) c++;
    return c;
}

qreal PaperDendrogramPlot::maxDistance() const {
    qreal mx = 0;
    for (const auto& e : entries_) mx = qMax(mx, e.distance);
    return mx;
}

QMap<QString, int> PaperDendrogramPlot::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperDendrogramPlot::onRender() {
    entries_.clear();
    int count = 5 + QRandomGenerator::global()->bounded(6);
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    QStringList categories = {"Hierarchical", "Agglomerative", "Divisive", "Spectral", "BIRCH"};
    QStringList branches = {"Branch-A", "Branch-B", "Branch-C", "Branch-D", "Branch-E"};
    for (int i = 0; i < count; ++i) {
        DendrogramEntry e;
        e.id = i + 1;
        e.label = inputField_->text().trimmed().isEmpty()
            ? QString("Cl_%1").arg(e.id)
            : inputField_->text().trimmed() + "_" + QString::number(e.id);
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.branch = branches[QRandomGenerator::global()->bounded(branches.size())];
        e.distance = QRandomGenerator::global()->bounded(1000) / 100.0;
        e.leaves = QRandomGenerator::global()->bounded(1, 50);
        e.root = (i == 0) || QRandomGenerator::global()->bounded(5) == 0;
        e.color = colors[i % colors.size()];
        entries_.append(e);
    }
    saveSettings();
    updateInfo();
    emit clusterSelected(entries_.last().id, entries_.last().distance);
    update();
}

void PaperDendrogramPlot::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Entries: 0 | Roots: 0 | Max Dist: 0.00");
    update();
}

void PaperDendrogramPlot::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));
    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render dendrogram");
        return;
    }
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Dendrogram Plot");
    int w = width(), h = height();
    drawDendrogram(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDendrogramPlot::drawDendrogram(QPainter& p, const QRect& rect) {
    qreal md = maxDistance();
    if (md <= 0) md = 1;
    int baseY = rect.bottom() - 10;
    int topY = rect.top() + 20;
    int n = qMin(entries_.size(), 12);
    int spacing = (rect.width() - 20) / qMax(n, 1);
    p.setPen(QColor(203, 213, 225));
    p.drawLine(rect.left() + 10, baseY, rect.right() - 10, baseY);
    for (int i = 0; i < n; ++i) {
        const auto& e = entries_[i];
        int x = rect.left() + 10 + i * spacing + spacing / 2;
        int y = baseY - static_cast<int>(e.distance / md * (baseY - topY));
        p.setPen(QPen(e.color, 2));
        p.drawLine(x, baseY, x, y);
        if (i < n - 1) {
            int nextY = baseY - static_cast<int>(entries_[i + 1].distance / md * (baseY - topY));
            p.drawLine(x, y, x + spacing, nextY);
        }
        p.setPen(Qt::NoPen);
        p.setBrush(e.root ? e.color : QColor(0xe2e8f0));
        p.drawEllipse(x - 5, y - 5, 10, 10);
        if (e.root) {
            p.setBrush(e.color.lighter(140));
            p.drawEllipse(x - 3, y - 3, 6, 6);
        }
        p.setPen(QColor(0x334155));
        p.setFont(QFont("Arial", 7));
        p.drawText(x - 20, baseY + 14, e.label.left(8));
    }
    p.setBrush(Qt::NoBrush);
}

void PaperDendrogramPlot::drawCategoryLegend(QPainter& p, const QRect& rect) {
    auto counts = categoryCounts();
    int y = rect.top() + 5;
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");
    y += 20;
    QList<QColor> colors = {QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0xdc2626), QColor(0x7c3aed)};
    int ci = 0;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        p.setPen(Qt::NoPen);
        p.setBrush(colors[ci % colors.size()]);
        p.drawRoundedRect(rect.x(), y, qMin(it.value() * 25, rect.width()), 16, 3, 3);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 5, y + 13, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += 22;
        ++ci;
    }
    p.setBrush(Qt::NoBrush);
}

void PaperDendrogramPlot::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries", QString::number(entries_.size()), QColor(0x3b82f6)},
        {"Roots", QString::number(rootCount()), QColor(0xdc2626)},
        {"Max Dist", QString::number(maxDistance(), 'f', 2), QColor(0x16a34a)},
        {"Categories", QString::number(categoryCounts().size()), QColor(0x7c3aed)}
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

void PaperDendrogramPlot::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Entries: 0 | Roots: 0 | Max Dist: 0.00"); return; }
    infoLabel_->setText(QString("Entries: %1 | Roots: %2 | Max Dist: %3")
        .arg(entries_.size()).arg(rootCount()).arg(QString::number(maxDistance(), 'f', 2)));
}

void PaperDendrogramPlot::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DendrogramEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.branch = settings_.value("branch").toString();
        e.distance = settings_.value("distance").toDouble();
        e.leaves = settings_.value("leaves").toInt();
        e.root = settings_.value("root").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDendrogramPlot::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("branch", entries_[i].branch);
        settings_.setValue("distance", entries_[i].distance);
        settings_.setValue("leaves", entries_[i].leaves);
        settings_.setValue("root", entries_[i].root);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
