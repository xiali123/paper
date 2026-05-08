#include "visualization/PaperTagCloudWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperTagCloudWidget::PaperTagCloudWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "TagCloud")
{
    setupUI();
}

void PaperTagCloudWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Layout:"));
    layoutCombo_ = new QComboBox();
    layoutCombo_->addItems({"Cloud", "Grid", "List"});
    connect(layoutCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTagCloudWidget::onLayoutChanged);
    toolbar->addWidget(layoutCombo_);

    toolbar->addWidget(new QLabel("Sort:"));
    sortCombo_ = new QComboBox();
    sortCombo_->addItems({"Count", "Name", "Weight"});
    connect(sortCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTagCloudWidget::onSortChanged);
    toolbar->addWidget(sortCombo_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTagCloudWidget::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Load papers to view tag cloud");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 420);
}

void PaperTagCloudWidget::setTags(const QMap<QString, int>& tagMap) {
    tags_.clear();
    QColor palette[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                        QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                        QColor(14,165,233), QColor(168,85,247), QColor(234,179,8), QColor(34,197,94)};
    int i = 0;
    for (auto it = tagMap.begin(); it != tagMap.end(); ++it) {
        TagInfo t;
        t.name = it.key();
        t.count = it.value();
        t.color = palette[i % 10];
        tags_.append(t);
        i++;
    }
    rebuildTags();
    updateInfo();
    emit tagsChanged(uniqueTagCount(), totalTagCount());
    update();
}

void PaperTagCloudWidget::addTag(const QString& name, int count) {
    for (auto& t : tags_) {
        if (t.name == name) { t.count += count; rebuildTags(); updateInfo(); update(); return; }
    }
    TagInfo t;
    t.name = name;
    t.count = count;
    QColor palette[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                        QColor(239,68,68), QColor(139,92,246)};
    t.color = palette[tags_.size() % 5];
    tags_.append(t);
    rebuildTags();
    updateInfo();
    emit tagsChanged(uniqueTagCount(), totalTagCount());
    update();
}

QList<TagInfo> PaperTagCloudWidget::tags() const { return tags_; }

QStringList PaperTagCloudWidget::topTags(int limit) const {
    QStringList result;
    QList<TagInfo> sorted = tags_;
    std::sort(sorted.begin(), sorted.end(),
        [](const TagInfo& a, const TagInfo& b) { return a.count > b.count; });
    for (int i = 0; i < qMin(limit, sorted.size()); ++i) result.append(sorted[i].name);
    return result;
}

int PaperTagCloudWidget::totalTagCount() const {
    int t = 0; for (const auto& tag : tags_) t += tag.count; return t;
}

int PaperTagCloudWidget::uniqueTagCount() const { return tags_.size(); }

void PaperTagCloudWidget::onLayoutChanged(int) { update(); }
void PaperTagCloudWidget::onSortChanged(int) { update(); }
void PaperTagCloudWidget::onClear() {
    tags_.clear();
    infoLabel_->setText("Load papers to view tag cloud");
    update();
}

void PaperTagCloudWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (tags_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Load papers to view tag cloud");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Tag Cloud");

    int w = width(), h = height();
    drawCloud(p, QRect(20, 50, w - 40, h / 2 - 30));
    drawBarChart(p, QRect(20, h / 2 + 20, w / 2 - 30, h / 2 - 50));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperTagCloudWidget::drawCloud(QPainter& p, const QRect& rect) {
    QList<TagInfo> sorted = tags_;
    int sortIdx = sortCombo_->currentIndex();
    if (sortIdx == 0) std::sort(sorted.begin(), sorted.end(), [](const TagInfo& a, const TagInfo& b) { return a.count > b.count; });
    else if (sortIdx == 1) std::sort(sorted.begin(), sorted.end(), [](const TagInfo& a, const TagInfo& b) { return a.name < b.name; });
    else std::sort(sorted.begin(), sorted.end(), [](const TagInfo& a, const TagInfo& b) { return a.weight > b.weight; });

    int maxCount = 1;
    for (const auto& t : sorted) maxCount = qMax(maxCount, t.count);

    int layoutIdx = layoutCombo_->currentIndex();
    int show = qMin(30, sorted.size());

    if (layoutIdx == 0) {
        // Cloud layout
        qreal cx = rect.x() + rect.width() / 2;
        qreal cy = rect.y() + rect.height() / 2;
        qreal angle = 0;
        qreal radius = 0;

        for (int i = 0; i < show; ++i) {
            qreal relSize = static_cast<qreal>(sorted[i].count) / maxCount;
            int fontSize = qBound(8, static_cast<int>(8 + relSize * 16), 22);
            QFont::Weight weight = relSize > 0.5 ? QFont::Bold : QFont::Normal;

            if (i > 0) {
                angle += 0.7;
                radius += 3;
            }
            qreal x = cx + radius * std::cos(angle) - 30;
            qreal y = cy + radius * std::sin(angle) * 0.6;

            p.setPen(sorted[i].color);
            p.setFont(QFont("Arial", fontSize, weight));
            p.drawText(QPointF(x, y), sorted[i].name);
        }
    } else if (layoutIdx == 1) {
        // Grid layout
        int cols = qMax(2, rect.width() / 100);
        int rows = (show + cols - 1) / cols;
        int cellW = rect.width() / cols;
        int cellH = qMin(24, (rect.height() - 10) / qMax(1, rows));

        for (int i = 0; i < show; ++i) {
            int col = i % cols;
            int row = i / cols;
            qreal relSize = static_cast<qreal>(sorted[i].count) / maxCount;
            int fontSize = qBound(8, static_cast<int>(9 + relSize * 8), 16);

            p.setPen(sorted[i].color);
            p.setFont(QFont("Arial", fontSize, relSize > 0.4 ? QFont::Bold : QFont::Normal));
            p.drawText(rect.x() + col * cellW + 8, rect.y() + 10 + row * cellH + cellH / 2, sorted[i].name);
        }
    } else {
        // List layout
        int itemH = qMin(20, (rect.height() - 10) / show);
        for (int i = 0; i < show; ++i) {
            qreal relSize = static_cast<qreal>(sorted[i].count) / maxCount;
            int fontSize = qBound(8, static_cast<int>(9 + relSize * 8), 16);

            p.setPen(sorted[i].color);
            p.setFont(QFont("Arial", fontSize, relSize > 0.4 ? QFont::Bold : QFont::Normal));
            p.drawText(rect.x() + 10, rect.y() + 10 + i * itemH + itemH, sorted[i].name);

            p.setPen(QColor(100, 116, 139));
            p.setFont(QFont("Arial", 8));
            p.drawText(rect.x() + rect.width() - 40, rect.y() + 10 + i * itemH + itemH,
                       QString::number(sorted[i].count));
        }
    }
}

void PaperTagCloudWidget::drawBarChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Top Tags");

    QList<TagInfo> sorted = tags_;
    std::sort(sorted.begin(), sorted.end(), [](const TagInfo& a, const TagInfo& b) { return a.count > b.count; });
    int show = qMin(8, sorted.size());
    if (show == 0) return;

    int maxCount = sorted.first().count;
    int barH = qMin(20, (rect.height() - 35) / show);

    for (int i = 0; i < show; ++i) {
        int y = rect.y() + 20 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(sorted[i].count) / maxCount) * (rect.width() - 110));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 2, 60, barH, Qt::AlignRight | Qt::AlignVCenter,
                   sorted[i].name.left(10));

        p.setPen(Qt::NoPen);
        p.setBrush(sorted[i].color);
        p.drawRoundedRect(rect.x() + 65, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 68 + barW, y + barH - 2, QString::number(sorted[i].count));
    }
}

void PaperTagCloudWidget::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Unique Tags", QString::number(uniqueTagCount()), QColor(59,130,246)},
        {"Total Count", QString::number(totalTagCount()), QColor(16,185,129)},
        {"Avg/Tag", QString::number(uniqueTagCount() > 0 ? static_cast<qreal>(totalTagCount()) / uniqueTagCount() : 0, 'f', 1), QColor(245,158,11)},
        {"Top Tag", topTags(1).isEmpty() ? "-" : topTags(1).first(), QColor(139,92,246)}
    };

    int boxH = qMin(45, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 24, Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperTagCloudWidget::updateInfo() {
    if (tags_.isEmpty()) { infoLabel_->setText("Load papers to view tag cloud"); return; }
    infoLabel_->setText(QString("%1 unique tags | %2 total | Top: %3")
        .arg(uniqueTagCount()).arg(totalTagCount())
        .arg(topTags(3).join(", ")));
}

void PaperTagCloudWidget::rebuildTags() {
    qreal maxCount = 1;
    for (const auto& t : tags_) maxCount = qMax(maxCount, static_cast<qreal>(t.count));
    for (auto& t : tags_) t.weight = static_cast<qreal>(t.count) / maxCount;
}
