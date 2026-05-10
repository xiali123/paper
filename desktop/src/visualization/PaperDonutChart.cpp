#include "visualization/PaperDonutChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperDonutChart::PaperDonutChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "DonutChart")
{
    setupUI();
    loadSettings();
}

void PaperDonutChart::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    generateBtn_ = new QPushButton("Generate");
    generateBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(generateBtn_, &QPushButton::clicked, this, &PaperDonutChart::onGenerate);
    toolbar->addWidget(generateBtn_);

    toolbar->addWidget(new QLabel("Segment:"));
    segmentCombo_ = new QComboBox();
    segmentCombo_->addItems({"All", "Category", "Year", "Journal", "Author"});
    toolbar->addWidget(segmentCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperDonutChart::onClear);
    toolbar->addWidget(clearBtn_);
    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter dataset name for donut chart...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Generate donut chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperDonutChart::addEntry(const DonutEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit chartGenerated(entry.id, entry.value);
    update();
}

QList<DonutEntry> PaperDonutChart::entries() const { return entries_; }

qreal PaperDonutChart::totalValue() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.value;
    return t;
}

int PaperDonutChart::topSegments() const {
    int c = 0;
    for (const auto& e : entries_) if (e.rank <= 3) c++;
    return c;
}

QMap<QString, int> PaperDonutChart::segmentCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.segment]++;
    return counts;
}

void PaperDonutChart::onGenerate() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"ML", "NLP", "CV", "Robotics", "Theory", "Data"};
    QStringList segments = {"primary", "secondary", "tertiary"};
    QColor segColors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                          QColor(239,68,68), QColor(139,92,246), QColor(236,72,153)};

    entries_.clear();
    int count = 4 + QRandomGenerator::global()->bounded(4);
    for (int i = 0; i < count; ++i) {
        DonutEntry e;
        e.id = entries_.size() + 1;
        e.category = categories[i % categories.size()];
        e.value = 10 + QRandomGenerator::global()->bounded(100);
        e.percentage = 0;
        e.label = e.category + " " + QString::number(e.value);
        e.itemCount = 5 + QRandomGenerator::global()->bounded(50);
        e.average = e.value / static_cast<qreal>(e.itemCount);
        int sIdx = i < 2 ? 0 : (i < 4 ? 1 : 2);
        e.segment = segments[sIdx];
        e.rank = i + 1;
        e.highlighted = i == 0;
        e.color = segColors[i % 6];
        entries_.append(e);
    }

    qreal total = 0;
    for (const auto& e : entries_) total += e.value;
    for (auto& e : entries_) e.percentage = e.value / qMax(total, 1.0);

    saveSettings();
    updateInfo();
    emit chartGenerated(entries_.size(), totalValue());
    update();
    inputField_->clear();
}

void PaperDonutChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Generate donut chart");
    update();
}

void PaperDonutChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Generate donut chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Donut Chart");

    int w = width(), h = height();
    drawDonutView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperDonutChart::drawDonutView(QPainter& p, const QRect& rect) {
    qreal total = totalValue();
    if (total == 0) return;

    int pieW = qMin(rect.width(), rect.height());
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + rect.height() / 2;

    qreal startAngle = 0;
    for (const auto& e : entries_) {
        qreal span = (e.value / total) * 360;
        p.setPen(e.highlighted ? QPen(Qt::white, 2) : Qt::NoPen);
        p.setBrush(e.color);
        p.drawPie(cx - pieW / 2, cy - pieW / 2, pieW, pieW,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));

        if (e.percentage > 0.08) {
            qreal midAngle = startAngle + span / 2;
            int tx = cx + static_cast<int>(pieW / 3 * qCos(midAngle * M_PI / 180));
            int ty = cy - static_cast<int>(pieW / 3 * qSin(midAngle * 16 * M_PI / 180 / 16));
            p.setPen(Qt::white);
            p.setFont(QFont("Arial", 8, QFont::Bold));
            p.drawText(tx - 20, ty - 5, 40, 14, Qt::AlignCenter,
                       QString::number(e.percentage * 100, 'f', 0) + "%");
        }
        startAngle += span;
    }

    p.setBrush(Qt::white);
    p.drawEllipse(cx - pieW / 4, cy - pieW / 4, pieW / 2, pieW / 2);

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 12, QFont::Bold));
    p.drawText(cx - 15, cy + 5, QString::number(static_cast<int>(total)));
}

void PaperDonutChart::drawLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Legend");

    int itemH = qMin(26, (rect.height() - 30) / qMax(entries_.size(), 1));
    int show = qMin(static_cast<int>(entries_.size()), (rect.height() - 30) / qMax(itemH, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + 22 + i * (itemH + 2);

        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 12, 12, 2, 2);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + 16, y + 1, rect.width() / 2 - 16, 14, Qt::AlignVCenter,
                   e.category);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() / 2, y + 1, rect.width() / 2 - 5, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.value) + " | " + QString::number(e.itemCount) + " items");
    }
}

void PaperDonutChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Segments", QString::number(entries_.size()), QColor(59,130,246)},
        {"Top 3", QString::number(topSegments()), QColor(16,185,129)},
        {"Total Value", QString::number(static_cast<int>(totalValue())), QColor(245,158,11)},
        {"Groups", QString::number(segmentCounts().size()), QColor(139,92,246)}
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

void PaperDonutChart::updateInfo() {
    if (entries_.isEmpty()) { infoLabel_->setText("Generate donut chart"); return; }
    infoLabel_->setText(QString("%1 segments | %2 total | %3 top")
        .arg(entries_.size()).arg(static_cast<int>(totalValue())).arg(topSegments()));
}

void PaperDonutChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        DonutEntry e;
        e.id = settings_.value("id").toInt();
        e.category = settings_.value("category").toString();
        e.value = settings_.value("value").toDouble();
        e.percentage = settings_.value("percentage").toDouble();
        e.label = settings_.value("label").toString();
        e.itemCount = settings_.value("itemCount").toInt();
        e.average = settings_.value("average").toDouble();
        e.segment = settings_.value("segment").toString();
        e.rank = settings_.value("rank").toInt();
        e.highlighted = settings_.value("highlighted").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperDonutChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("value", entries_[i].value);
        settings_.setValue("percentage", entries_[i].percentage);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("itemCount", entries_[i].itemCount);
        settings_.setValue("average", entries_[i].average);
        settings_.setValue("segment", entries_[i].segment);
        settings_.setValue("rank", entries_[i].rank);
        settings_.setValue("highlighted", entries_[i].highlighted);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
