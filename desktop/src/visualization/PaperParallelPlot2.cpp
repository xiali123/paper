#include "visualization/PaperParallelPlot2.hpp"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QRandomGenerator>

PaperParallelPlot2::PaperParallelPlot2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "ParallelPlot2")
{
    setupUI();
    loadSettings();
}

void PaperParallelPlot2::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Metrics", "Features", "Scores", "Dimensions", "Attributes"});

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Entry label...");

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperParallelPlot2::onRender);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperParallelPlot2::onClear);

    infoLabel_ = new QLabel("Entries: 0 | Highlights: 0 | Max V1: 0.00");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");

    mainLayout->addWidget(categoryCombo_);
    mainLayout->addWidget(inputField_);
    mainLayout->addWidget(renderBtn_);
    mainLayout->addWidget(clearBtn_);
    mainLayout->addWidget(infoLabel_);
    mainLayout->addStretch();

    setMinimumSize(600, 500);
}

void PaperParallelPlot2::addEntry(const ParallelEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    update();
}

QList<ParallelEntry> PaperParallelPlot2::entries() const {
    return entries_;
}

int PaperParallelPlot2::highlightCount() const {
    int count = 0;
    for (const auto& e : entries_) {
        if (e.highlight) ++count;
    }
    return count;
}

qreal PaperParallelPlot2::maxV1() const {
    qreal maxVal = 0;
    for (const auto& e : entries_) {
        if (e.v1 > maxVal) maxVal = e.v1;
    }
    return maxVal;
}

QMap<QString, int> PaperParallelPlot2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) {
        counts[e.category]++;
    }
    return counts;
}

void PaperParallelPlot2::onRender() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    ParallelEntry entry;
    entry.id = entries_.size() + 1;
    entry.label = text;
    entry.category = categoryCombo_->currentText();
    entry.axis = QString("Axis_%1").arg((entry.id - 1) / 3 + 1);
    entry.v1 = 1 + QRandomGenerator::global()->bounded(100.0);
    entry.v2 = 1 + QRandomGenerator::global()->bounded(100.0);
    entry.v3 = 1 + QRandomGenerator::global()->bounded(100.0);

    qreal mv = 0;
    for (const auto& e : entries_) { if (e.v1 > mv) mv = e.v1; }
    entry.highlight = (entry.v1 >= mv);

    for (auto& e : entries_) {
        e.highlight = false;
    }
    entry.highlight = true;

    entry.color = palette[entries_.size() % 5];

    entries_.append(entry);

    saveSettings();
    updateInfo();
    emit lineSelected(entry.id, entry.v1);
    update();
    inputField_->clear();
}

void PaperParallelPlot2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperParallelPlot2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render parallel plot");
        return;
    }

    int w = width(), h = height();
    int topMargin = 50;
    int colW = (w - 60) / 3;

    drawParallelView(p, QRect(20, topMargin, colW, h - topMargin - 20));
    drawCategoryLegend(p, QRect(20 + colW + 10, topMargin, colW, h - topMargin - 20));
    drawStats(p, QRect(20 + 2 * (colW + 10), topMargin, colW, h - topMargin - 20));
}

void PaperParallelPlot2::drawParallelView(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Parallel Coordinates");

    if (entries_.isEmpty()) return;

    int x1 = rect.left() + 30;
    int x2 = rect.left() + rect.width() / 2;
    int x3 = rect.right() - 30;

    p.setPen(QPen(QColor(0x94a3b8), 1));
    p.drawLine(x1, rect.top() + 30, x1, rect.bottom());
    p.drawLine(x2, rect.top() + 30, x2, rect.bottom());
    p.drawLine(x3, rect.top() + 30, x3, rect.bottom());

    p.setPen(QColor(0x334155));
    p.setFont(QFont("Arial", 8));
    p.drawText(x1 - 8, rect.top() + 26, "V1");
    p.drawText(x2 - 8, rect.top() + 26, "V2");
    p.drawText(x3 - 8, rect.top() + 26, "V3");

    int drawH = rect.height() - 40;
    int limit = qMin(entries_.size(), 15);
    for (int i = 0; i < limit; ++i) {
        const auto& e = entries_[i];
        int y1 = rect.top() + 30 + static_cast<int>((1.0 - e.v1 / 100.0) * drawH);
        int y2 = rect.top() + 30 + static_cast<int>((1.0 - e.v2 / 100.0) * drawH);
        int y3 = rect.top() + 30 + static_cast<int>((1.0 - e.v3 / 100.0) * drawH);

        p.setPen(QPen(e.color, e.highlight ? 3 : 1));
        p.drawLine(x1, y1, x2, y2);
        p.drawLine(x2, y2, x3, y3);

        if (e.highlight) {
            p.setBrush(e.color);
            p.drawEllipse(x1 - 3, y1 - 3, 6, 6);
            p.drawEllipse(x2 - 3, y2 - 3, 6, 6);
            p.drawEllipse(x3 - 3, y3 - 3, 6, 6);
            p.setBrush(Qt::NoBrush);
        }
    }
}

void PaperParallelPlot2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 18, "Categories");

    auto counts = categoryCounts();
    static const QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    int y = rect.y() + 30;
    int itemH = 24;
    int maxItems = (rect.height() - 30) / itemH;
    int ci = 0;

    for (auto it = counts.begin(); it != counts.end() && ci < maxItems; ++it, ++ci) {
        p.setPen(Qt::NoPen);
        p.setBrush(palette[ci % 5]);
        p.drawRoundedRect(rect.x(), y, qMin(it.value() * 20, rect.width()), 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 4, y + 12, QString("%1: %2").arg(it.key()).arg(it.value()));
        y += itemH;
    }

    p.setBrush(Qt::NoBrush);
}

void PaperParallelPlot2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Entries",  QString::number(entries_.size()), QColor("#3b82f6")},
        {"Highlights",     QString::number(highlightCount()), QColor("#16a34a")},
        {"Max V1",         QString::number(maxV1(), 'f', 1),  QColor("#d97706")}
    };

    int boxH = qMin(50, (rect.height() - 10) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 6);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22, Qt::AlignVCenter,
                   stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 28, rect.width() - 20, 14, Qt::AlignVCenter,
                   stats[i].label);
    }
}

void PaperParallelPlot2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Entries: 0 | Highlights: 0 | Max V1: 0.00");
        return;
    }
    infoLabel_->setText(
        QString("Entries: %1 | Highlights: %2 | Max V1: %3")
            .arg(entries_.size())
            .arg(highlightCount())
            .arg(maxV1(), 0, 'f', 2));
}

void PaperParallelPlot2::loadSettings() {
    settings_.beginGroup("ParallelPlot2");
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        ParallelEntry e;
        e.id = settings_.value("id").toInt();
        e.label = settings_.value("label").toString();
        e.category = settings_.value("category").toString();
        e.axis = settings_.value("axis").toString();
        e.v1 = settings_.value("v1").toDouble();
        e.v2 = settings_.value("v2").toDouble();
        e.v3 = settings_.value("v3").toDouble();
        e.highlight = settings_.value("highlight").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    settings_.endGroup();
    updateInfo();
}

void PaperParallelPlot2::saveSettings() {
    settings_.beginGroup("ParallelPlot2");
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("label", entries_[i].label);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("axis", entries_[i].axis);
        settings_.setValue("v1", entries_[i].v1);
        settings_.setValue("v2", entries_[i].v2);
        settings_.setValue("v3", entries_[i].v3);
        settings_.setValue("highlight", entries_[i].highlight);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
    settings_.endGroup();
}
