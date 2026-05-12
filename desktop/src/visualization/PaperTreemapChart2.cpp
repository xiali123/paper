#include "visualization/PaperTreemapChart2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <algorithm>

namespace {
static const QColor kPalette[] = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};
static const QStringList kCategories = {"AI", "Data", "Systems", "Theory", "Applied"};
static const QStringList kLabels = {
    "Machine Learning", "Deep Learning", "NLP", "CV",
    "Reinforcement Learning"
};
static const QStringList kParents = {"Root", "ML", "NLP"};

int categoryIndex(const QString& cat) {
    int idx = kCategories.indexOf(cat);
    return idx >= 0 ? idx : 0;
}
}

PaperTreemapChart2::PaperTreemapChart2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TreemapChart2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        entries_.clear();

        auto make = [](int id, const QString& label, const QString& cat,
                       const QString& parent, qreal area, int depth,
                       bool hl) -> TreemapChart2Entry {
            return {id, label, cat, parent, area, depth, hl,
                    kPalette[categoryIndex(cat)]};
        };

        entries_ = {
            make(1, "Machine Learning",       "AI",      "Root", 85.0, 0, false),
            make(2, "Deep Learning",           "AI",      "ML",   60.0, 1, true),
            make(3, "NLP",                     "Data",    "Root", 45.0, 0, false),
            make(4, "CV",                      "Systems", "ML",   55.0, 1, true),
            make(5, "Reinforcement Learning",  "Theory",  "Root", 35.0, 0, false),
            make(6, "Transfer Learning",       "Applied", "ML",   25.0, 1, false),
            make(7, "Transformer Models",      "Data",    "NLP",  40.0, 2, true),
            make(8, "Generative AI",           "AI",      "ML",   50.0, 1, false),
        };
        saveSettings();
        updateInfo();
        update();
    }
}

void PaperTreemapChart2::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "AI", "Data", "Systems", "Theory", "Applied"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Search labels...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    layoutBtn_ = new QPushButton("Layout");
    layoutBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(layoutBtn_, &QPushButton::clicked, this, &PaperTreemapChart2::onLayout);
    toolbar->addWidget(layoutBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTreemapChart2::onClear);
    toolbar->addWidget(clearBtn_);

    toolbar->addStretch();

    infoLabel_ = new QLabel("Treemap Chart 2");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    infoLabel_->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    toolbar->addWidget(infoLabel_);

    layout->addLayout(toolbar);
    setMinimumSize(700, 500);
}

void PaperTreemapChart2::addEntry(const TreemapChart2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit cellSelected(entry.id, entry.area);
    update();
}

QList<TreemapChart2Entry> PaperTreemapChart2::entries() const { return entries_; }

int PaperTreemapChart2::highlightCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.highlight) ++c;
    return c;
}

qreal PaperTreemapChart2::totalArea() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.area;
    return t;
}

QMap<QString, int> PaperTreemapChart2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

// ---------- squarified treemap helpers ----------
namespace {

struct CellRect { int index; QRectF rect; };

void squarify(QList<CellRect>& out, QList<int>& indices,
              const QRectF& r, const QList<TreemapChart2Entry>& entries)
{
    if (indices.isEmpty()) return;
    if (r.width() < 1 || r.height() < 1) return;

    qreal total = 0;
    for (int i : indices) total += entries[i].area;
    if (total <= 0) return;

    bool vertical = r.width() >= r.height();
    qreal side = vertical ? r.height() : r.width();

    QList<int> row;
    qreal rowSum = 0;
    qreal bestWorst = 1e9;
    int bestEnd = 0;

    for (int j = 0; j < indices.size(); ++j) {
        row.append(indices[j]);
        rowSum += entries[indices[j]].area;

        qreal rowFrac = rowSum / total;
        qreal rowSide = rowFrac * side;

        qreal worst = 0;
        for (int k : row) {
            qreal kFrac = entries[k].area / rowSum;
            qreal kOther = kFrac * rowSide;
            qreal ratio = (kOther > 0) ? (rowSide / kOther) : 1e9;
            if (ratio < 1) ratio = 1.0 / ratio;
            worst = qMax(worst, ratio);
        }

        if (worst <= bestWorst) {
            bestWorst = worst;
            bestEnd = j + 1;
        } else {
            break;
        }
    }

    QList<int> rowIndices;
    for (int j = 0; j < bestEnd; ++j) rowIndices.append(indices[j]);

    qreal rowTotal = 0;
    for (int i : rowIndices) rowTotal += entries[i].area;
    qreal rowFrac = rowTotal / total;

    QRectF rowRect;
    QRectF remainder;
    if (vertical) {
        qreal rowW = r.width() * rowFrac;
        rowRect = QRectF(r.x(), r.y(), rowW, r.height());
        remainder = QRectF(r.x() + rowW, r.y(), r.width() - rowW, r.height());
    } else {
        qreal rowH = r.height() * rowFrac;
        rowRect = QRectF(r.x(), r.y(), r.width(), rowH);
        remainder = QRectF(r.x(), r.y() + rowH, r.width(), r.height() - rowH);
    }

    qreal offset = 0;
    qreal rowSide = vertical ? rowRect.height() : rowRect.width();
    for (int i : rowIndices) {
        qreal frac = entries[i].area / rowTotal;
        qreal span = frac * rowSide;
        QRectF cr;
        if (vertical)
            cr = QRectF(rowRect.x(), rowRect.y() + offset, rowRect.width(), span);
        else
            cr = QRectF(rowRect.x() + offset, rowRect.y(), span, rowRect.height());
        out.append({i, cr});
        offset += span;
    }

    QList<int> rest;
    for (int j = bestEnd; j < indices.size(); ++j) rest.append(indices[j]);
    squarify(out, rest, remainder, entries);
}

} // anonymous namespace

// ---------- slots ----------
void PaperTreemapChart2::onLayout() {
    int id = entries_.size() + 1;
    QString label = kLabels[QRandomGenerator::global()->bounded(kLabels.size())];
    QString cat = kCategories[QRandomGenerator::global()->bounded(kCategories.size())];
    QString parent = kParents[QRandomGenerator::global()->bounded(kParents.size())];
    qreal area = 10.0 + QRandomGenerator::global()->bounded(90);
    int depth = QRandomGenerator::global()->bounded(3);
    bool hl = QRandomGenerator::global()->bounded(3) == 0;

    TreemapChart2Entry e{id, label, cat, parent, area, depth, hl,
                         kPalette[categoryIndex(cat)]};
    addEntry(e);
}

void PaperTreemapChart2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

// ---------- painting ----------
void PaperTreemapChart2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Layout treemap chart");
        return;
    }

    int w = width(), h = height();
    int toolbarH = 40;
    int topY = toolbarH;
    int bottomSplit = h * 3 / 4; // top 75% for treemap+legend, bottom 25% stats

    // treemap: left 60%
    int treemapW = w * 3 / 5;
    drawTreemap(p, QRect(10, topY, treemapW - 10, bottomSplit - topY));

    // category legend: right 40%
    drawCategoryLegend(p, QRect(treemapW, topY, w - treemapW - 10, bottomSplit - topY));

    // stats: bottom 25%
    drawStats(p, QRect(10, bottomSplit + 5, w - 20, h - bottomSplit - 15));
}

void PaperTreemapChart2::drawTreemap(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Treemap");

    QRectF canvas(rect.x() + 5, rect.y() + 24, rect.width() - 10, rect.height() - 30);
    if (canvas.width() < 20 || canvas.height() < 20) return;

    // filter by category combo
    QString catFilter = categoryCombo_->currentText();
    QList<int> visible;
    for (int i = 0; i < entries_.size(); ++i) {
        if (catFilter == "All" || entries_[i].category == catFilter)
            visible.append(i);
    }
    if (visible.isEmpty()) return;

    // sort by area descending for better layout
    std::sort(visible.begin(), visible.end(),
              [this](int a, int b) { return entries_[a].area > entries_[b].area; });

    QList<CellRect> cells;
    squarify(cells, visible, canvas, entries_);

    for (const auto& c : cells) {
        const auto& e = entries_[c.index];
        QRectF cr = c.rect.adjusted(1, 1, -1, -1);
        if (cr.width() < 2 || cr.height() < 2) continue;

        QColor fill = e.color;
        fill.setAlpha(180);
        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawRoundedRect(cr, 4, 4);

        if (e.highlight) {
            p.setPen(QPen(QColor("#ffd700"), 2.5));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(cr.adjusted(1, 1, -1, -1), 4, 4);
        }

        p.setPen(Qt::white);
        int fontSize = qMax(7, qMin(11, static_cast<int>(cr.height() / 4)));
        p.setFont(QFont("Arial", fontSize, QFont::Bold));
        p.drawText(cr.adjusted(4, 3, -4, -cr.height() / 2),
                   Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                   e.label);

        p.setPen(QColor(255, 255, 255, 200));
        p.setFont(QFont("Arial", qMax(6, fontSize - 2)));
        p.drawText(cr.adjusted(4, cr.height() / 2, -4, -3),
                   Qt::AlignLeft | Qt::AlignBottom,
                   QString("area: %1").arg(e.area, 0, 'f', 0));
    }
}

void PaperTreemapChart2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 16, "Categories");

    auto counts = categoryCounts();
    QString labels[] = {"AI", "Data", "Systems", "Theory", "Applied"};

    int itemH = qMin(28, (rect.height() - 40) / 5);
    int startY = rect.y() + 28;

    for (int i = 0; i < 5; ++i) {
        int y = startY + i * (itemH + 4);
        int count = counts.contains(labels[i]) ? counts[labels[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(kPalette[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, labels[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " items");
    }
}

void PaperTreemapChart2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Cells",   QString::number(entries_.size()), QColor("#3b82f6")},
        {"Highlighted",   QString::number(highlightCount()), QColor("#16a34a")},
        {"Total Area",    QString::number(totalArea(), 'f', 0), QColor("#d97706")},
        {"Avg Depth",     QString::number(
            entries_.isEmpty() ? 0.0
                : std::accumulate(entries_.begin(), entries_.end(), 0.0,
                      [](qreal s, const TreemapChart2Entry& e) { return s + e.depth; })
                    / entries_.size(),
            'f', 1),
         QColor("#dc2626")}
    };

    int boxH = qMin(50, (rect.height() - 10) / 4);
    int boxW = (rect.width() - 30) / 4;

    for (int i = 0; i < stats.size(); ++i) {
        int x = rect.x() + i * (boxW + 10);
        int y = rect.y();

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(x + 10, y + 5, boxW - 20, 22, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(x + 10, y + 28, boxW - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTreemapChart2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Layout treemap chart");
        return;
    }
    infoLabel_->setText(
        QString("%1 cells | %2 highlighted | %3 area")
            .arg(entries_.size())
            .arg(highlightCount())
            .arg(totalArea(), 0, 'f', 0));
}

void PaperTreemapChart2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TreemapChart2Entry e;
        e.id        = settings_.value("id").toInt();
        e.label     = settings_.value("label").toString();
        e.category  = settings_.value("category").toString();
        e.parent    = settings_.value("parent").toString();
        e.area      = settings_.value("area").toDouble();
        e.depth     = settings_.value("depth").toInt();
        e.highlight = settings_.value("highlight").toBool();
        e.color     = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTreemapChart2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",        entries_[i].id);
        settings_.setValue("label",     entries_[i].label);
        settings_.setValue("category",  entries_[i].category);
        settings_.setValue("parent",    entries_[i].parent);
        settings_.setValue("area",      entries_[i].area);
        settings_.setValue("depth",     entries_[i].depth);
        settings_.setValue("highlight", entries_[i].highlight);
        settings_.setValue("color",     entries_[i].color.name());
    }
    settings_.endArray();
}
