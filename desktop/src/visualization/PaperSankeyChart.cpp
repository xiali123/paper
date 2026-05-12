#include "visualization/PaperSankeyChart.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRegularExpression>
#include <QtMath>

PaperSankeyChart::PaperSankeyChart(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SankeyChart")
{
    setupUI();
    loadSettings();
}

void PaperSankeyChart::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperSankeyChart::onRender);
    toolbar->addWidget(renderBtn_);

    toolbar->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Citation", "Reference", "Influence", "Topic"});
    toolbar->addWidget(categoryCombo_, 1);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSankeyChart::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("source->target:value (e.g. Search->PDF:75)");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    layout->addWidget(inputField_);

    infoLabel_ = new QLabel("Render Sankey chart");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(600, 500);
}

void PaperSankeyChart::addEntry(const SankeyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit sankeyRendered(entry.id, entry.flow);
    update();
}

QList<SankeyEntry> PaperSankeyChart::entries() const {
    return entries_;
}

int PaperSankeyChart::mainFlowCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.mainFlow) ++c;
    return c;
}

qreal PaperSankeyChart::totalFlow() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.flow;
    return t;
}

QMap<QString, int> PaperSankeyChart::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSankeyChart::onRender() {
    static const QVector<QColor> palette = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    // Parse multiple entries separated by semicolons
    QStringList parts = text.split(';', Qt::SkipEmptyParts);
    int cIdx = categoryCombo_->currentIndex();
    static const QStringList categories = {"citation", "reference", "influence", "topic"};

    for (const QString& part : parts) {
        QString segment = part.trimmed();

        // Match pattern: source->target:value
        QRegularExpression re(R"((.+?)\s*->\s*(.+?)\s*:\s*(\d+(?:\.\d+)?)\s*)");
        QRegularExpressionMatch match = re.match(segment);

        SankeyEntry e;
        e.id = entries_.size() + 1;

        if (match.hasMatch()) {
            e.source = match.captured(1).trimmed();
            e.target = match.captured(2).trimmed();
            e.value = match.captured(3).toDouble();
        } else {
            // Fallback: use segment as source, generate target
            e.source = segment;
            e.target = "Output-" + QString::number(e.id);
            e.value = 10.0 + (e.id * 7.3);
        }

        e.flow = e.value;
        e.efficiency = e.flow > 0.0 ? qMin(e.flow / 100.0, 1.0) : 0.0;
        e.mainFlow = e.flow > 50.0;

        if (cIdx > 0 && cIdx <= categories.size()) {
            e.category = categories[cIdx - 1];
        } else {
            e.category = categories[e.id % categories.size()];
        }

        e.color = palette[e.id % palette.size()];

        addEntry(e);
    }

    inputField_->clear();
}

void PaperSankeyChart::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Render Sankey chart");
    update();
}

void PaperSankeyChart::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render Sankey chart");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Sankey Chart");

    int w = width(), h = height();
    drawSankeyView(p, QRect(20, 50, w / 2 - 20, h - 80));
    drawCategoryLegend(p, QRect(w / 2 + 10, 50, w / 2 - 30, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperSankeyChart::drawSankeyView(QPainter& p, const QRect& rect) {
    int n = entries_.size();
    int show = qMin(8, n);
    qreal maxFlow = 1.0;
    for (int i = 0; i < show; ++i)
        maxFlow = qMax(maxFlow, entries_[i].flow);

    int leftX = rect.x() + 60;
    int rightX = rect.x() + rect.width() - 80;
    int yStart = rect.y() + 10;
    int spacing = qMin(30, (rect.height() - 20) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int curY = yStart + i * spacing;
        qreal ratio = e.flow / maxFlow;
        int bandH = qMax(4, static_cast<int>(ratio * spacing * 0.8));

        int alpha = 80 + static_cast<int>(ratio * 120);
        QColor fill = QColor(e.color.red(), e.color.green(), e.color.blue(), alpha);

        // Draw the S-curve sankey path
        int midX = leftX + (rightX - leftX) / 2;
        QPainterPath path;
        path.moveTo(leftX, curY);
        path.cubicTo(midX, curY,
                     midX, curY + bandH,
                     rightX, curY + bandH / 2);
        path.cubicTo(midX, curY + bandH,
                     midX, curY,
                     leftX, curY + bandH);

        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(path);

        // Source label (left side)
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(leftX - 58, curY - 2, 55, bandH + 4,
                   Qt::AlignVCenter | Qt::AlignRight, e.source.left(10));

        // Target label (right side)
        p.drawText(rightX + 4, curY - 2, 70, bandH + 4,
                   Qt::AlignVCenter | Qt::AlignLeft, e.target.left(10));

        // Flow value on the band midpoint
        if (bandH >= 10) {
            p.setPen(QColor(255, 255, 255, 200));
            p.setFont(QFont("Arial", 6, QFont::Bold));
            int labelX = (leftX + rightX) / 2 - 12;
            int labelY = curY + bandH / 2 - 4;
            p.drawText(labelX, labelY, 24, 10, Qt::AlignCenter,
                       QString::number(static_cast<int>(e.flow)));
        }
    }
}

void PaperSankeyChart::drawCategoryLegend(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    static const QStringList categories = {"citation", "reference", "influence", "topic"};
    static const QString labels[] = {"Citation", "Reference", "Influence", "Topic"};
    static const QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#7c3aed")
    };

    int itemH = qMin(28, (rect.height() - 30) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 22 + i * (itemH + 4);
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;

        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 4, 14, 14, 3, 3);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y + 2, rect.width() / 2 - 24, 18,
                   Qt::AlignVCenter, labels[i]);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y + 2, rect.width() / 2 - 5, 18,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " flows");
    }
}

void PaperSankeyChart::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Entries",   QString::number(entries_.size()),  QColor("#3b82f6")},
        {"Main Flow", QString::number(mainFlowCount()),  QColor("#16a34a")},
        {"Total",     QString::number(totalFlow(), 'f', 0), QColor("#d97706")},
        {"Categories",QString::number(categoryCounts().size()), QColor("#7c3aed")}
    };

    int boxH = qMin(42, (rect.height() - 10) / 4);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + i * (boxH + 5);

        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(rect.x() + 10, y + 5, rect.width() - 20, 22,
                   Qt::AlignVCenter, stats[i].value);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x() + 10, y + 26, rect.width() - 20, 14,
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperSankeyChart::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Render Sankey chart");
        return;
    }
    infoLabel_->setText(QString("%1 entries | %2 main | %3 total flow")
        .arg(entries_.size())
        .arg(mainFlowCount())
        .arg(totalFlow(), 0, 'f', 0));
}

void PaperSankeyChart::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SankeyEntry e;
        e.id         = settings_.value("id").toInt();
        e.source     = settings_.value("source").toString();
        e.target     = settings_.value("target").toString();
        e.category   = settings_.value("category").toString();
        e.value      = settings_.value("value").toDouble();
        e.flow       = settings_.value("flow").toDouble();
        e.efficiency = settings_.value("efficiency").toDouble();
        e.mainFlow   = settings_.value("mainFlow").toBool();
        e.color      = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSankeyChart::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id",         entries_[i].id);
        settings_.setValue("source",     entries_[i].source);
        settings_.setValue("target",     entries_[i].target);
        settings_.setValue("category",   entries_[i].category);
        settings_.setValue("value",      entries_[i].value);
        settings_.setValue("flow",       entries_[i].flow);
        settings_.setValue("efficiency", entries_[i].efficiency);
        settings_.setValue("mainFlow",   entries_[i].mainFlow);
        settings_.setValue("color",      entries_[i].color.name());
    }
    settings_.endArray();
}
