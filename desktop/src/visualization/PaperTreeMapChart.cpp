#include "visualization/PaperTreeMapChart.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

namespace {
const QVector<QColor> kColors = {
    QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
    QColor("#dc2626"), QColor("#7c3aed")
};
const QStringList kCategories = {"Topic", "Author", "Year", "Venue", "Method"};
const QStringList kParentNames = {
    "Root", "Machine Learning", "Natural Language Processing",
    "Computer Vision", "Reinforcement Learning", "Deep Learning"
};
} // namespace

PaperTreeMapChart::PaperTreeMapChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TreeMapChart")
{
    setupUI();
    loadSettings();
}

void PaperTreeMapChart::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems(kCategories);
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 4px 8px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter label...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperTreeMapChart::onRender);
    toolbar->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTreeMapChart::onClear);
    toolbar->addWidget(clearBtn_);

    mainLayout->addLayout(toolbar);

    infoLabel_ = new QLabel("Render tree map");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    mainLayout->addWidget(infoLabel_);

    mainLayout->addStretch(1);
    setMinimumSize(700, 500);
}

void PaperTreeMapChart::addEntry(const TreeMapEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<TreeMapEntry> PaperTreeMapChart::entries() const { return entries_; }

int PaperTreeMapChart::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.dominant) ++c;
    return c;
}

qreal PaperTreeMapChart::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

QMap<QString, int> PaperTreeMapChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperTreeMapChart::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    TreeMapEntry e;
    e.id = entries_.size() + 1;
    e.label = text;
    e.category = categoryCombo_->currentText();
    e.size = 1 + QRandomGenerator::global()->bounded(100);
    e.value = e.size;
    e.depth = QRandomGenerator::global()->bounded(4); // 0-3
    e.dominant = e.size > 70;
    e.parent = kParentNames[QRandomGenerator::global()->bounded(kParentNames.size())];
    e.color = kColors[categoryCombo_->currentIndex() % kColors.size()];

    entries_.append(e);
    saveSettings();
    updateInfo();
    emit rectSelected(e.id, e.value);
    update();
    inputField_->clear();
}

void PaperTreeMapChart::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperTreeMapChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render tree map");
        return;
    }

    int w = width(), h = height();
    drawTreeMapView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTreeMapChart::drawTreeMapView(QPainter& p, const QRect& area) {
    // Section title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(area.x(), area.y() - 10, "Tree Map");

    // Compute total value for proportional sizing
    qreal total = totalValue();
    if (total <= 0) return;

    // Simple squarified-style treemap: lay out rectangles in alternating
    // horizontal/vertical strips proportional to each entry's value.
    QRect remaining = area.adjusted(2, 2, -2, -2);
    bool horizontal = true;

    int idx = 0;
    while (idx < entries_.size() && remaining.isValid()) {
        // Determine how many items fit in this strip (up to a reasonable batch)
        qreal remainingValue = 0;
        for (int i = idx; i < entries_.size(); ++i)
            remainingValue += entries_[i].value;
        if (remainingValue <= 0) break;

        // Pick a batch size: items that together take ~half the remaining space
        qreal batchTarget = remainingValue * 0.5;
        qreal batchSum = 0;
        int batchEnd = idx;
        while (batchEnd < entries_.size() && batchSum < batchTarget) {
            batchSum += entries_[batchEnd].value;
            ++batchEnd;
        }
        if (batchEnd == idx) batchEnd = idx + 1; // at least one

        qreal batchFraction = batchSum / remainingValue;
        int stripThickness;
        if (horizontal) {
            stripThickness = static_cast<int>(remaining.height() * batchFraction);
            stripThickness = qMax(stripThickness, 1);
        } else {
            stripThickness = static_cast<int>(remaining.width() * batchFraction);
            stripThickness = qMax(stripThickness, 1);
        }

        QRect stripRect;
        if (horizontal) {
            stripRect = QRect(remaining.x(), remaining.y(),
                              remaining.width(), stripThickness);
            remaining.setTop(remaining.y() + stripThickness);
        } else {
            stripRect = QRect(remaining.x(), remaining.y(),
                              stripThickness, remaining.height());
            remaining.setLeft(remaining.x() + stripThickness);
        }

        // Divide the strip among items in the batch
        int cursor = horizontal ? stripRect.x() : stripRect.y();
        int stripSpan = horizontal ? stripRect.width() : stripRect.height();
        for (int i = idx; i < batchEnd && i < entries_.size(); ++i) {
            const auto& entry = entries_[i];
            qreal fraction = entry.value / batchSum;
            int span = static_cast<int>(stripSpan * fraction);
            span = qMax(span, 2);

            QRect cellRect;
            if (horizontal) {
                cellRect = QRect(cursor, stripRect.y(), span, stripRect.height());
            } else {
                cellRect = QRect(stripRect.x(), cursor, stripRect.width(), span);
            }

            // Draw cell
            QColor fill = entry.color;
            fill.setAlphaF(0.70);
            p.setPen(Qt::NoPen);
            p.setBrush(fill);
            p.drawRoundedRect(cellRect.adjusted(1, 1, -1, -1), 4, 4);

            // Border
            p.setPen(QColor(15, 23, 42).lighter(140));
            p.setBrush(Qt::NoBrush);
            p.drawRoundedRect(cellRect.adjusted(1, 1, -1, -1), 4, 4);

            // Label inside
            if (cellRect.width() > 30 && cellRect.height() > 18) {
                p.setPen(QColor(255, 255, 255));
                p.setFont(QFont("Arial", 8, QFont::Bold));
                QString lbl = entry.label;
                if (lbl.length() > 10) lbl = lbl.left(9) + QStringLiteral("..");
                p.drawText(cellRect.adjusted(3, 3, -3, -3),
                           Qt::AlignTop | Qt::AlignLeft, lbl);

                // Size value
                p.setFont(QFont("Arial", 7));
                p.drawText(cellRect.adjusted(3, 3, -3, -3),
                           Qt::AlignBottom | Qt::AlignRight,
                           QString::number(static_cast<int>(entry.size)));
            }

            cursor += span;
        }

        idx = batchEnd;
        horizontal = !horizontal;
    }
}

void PaperTreeMapChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.topLeft(), "Legend");

    auto counts = categoryCounts();
    int itemH = qMin(28, (rect.height() - 30) / kCategories.size());

    for (int i = 0; i < kCategories.size(); ++i) {
        int y = rect.y() + 24 + i * (itemH + 4);
        int count = counts.contains(kCategories[i]) ? counts[kCategories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(kColors[i % kColors.size()]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, kCategories[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " nodes");
    }
}

void PaperTreeMapChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Nodes",  QString::number(entries_.size()),   QColor("#3b82f6")},
        {"Dominant",     QString::number(dominantCount()),   QColor("#dc2626")},
        {"Total Value",  QString::number(totalValue(), 'f', 1), QColor("#d97706")}
    };

    int boxH = qMin(48, (rect.height() - 10) / stats.size());
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 6);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 24,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 16,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTreeMapChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render tree map");
        return;
    }
    infoLabel_->setText(
        QString("Nodes: %1 | Dominant: %2 | Total: %3")
            .arg(entries_.size())
            .arg(dominantCount())
            .arg(totalValue(), 0, 'f', 1));
}

void PaperTreeMapChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        TreeMapEntry e;
        e.id       = settings_.value("id").toInt();
        e.label    = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.parent   = settings_.value("parent").toString();
        e.size     = settings_.value("size").toDouble();
        e.value    = settings_.value("value").toDouble();
        e.depth    = settings_.value("depth").toInt();
        e.dominant = settings_.value("dominant").toBool();
        e.color    = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperTreeMapChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",       entries_[i].id);
        settings_.setValue("label",    entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("parent",   entries_[i].parent);
        settings_.setValue("size",     entries_[i].size);
        settings_.setValue("value",    entries_[i].value);
        settings_.setValue("depth",    entries_[i].depth);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color",    entries_[i].color.name());
    }
    settings_.endArray();
}
