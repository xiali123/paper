#include "visualization/PaperSankeyDiagram2.hpp"
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperSankeyDiagram2::PaperSankeyDiagram2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "SankeyDiagram2")
{
    setupUI();
    loadSettings();
}

void PaperSankeyDiagram2::setupUI() {
    auto* layout = new QHBoxLayout(this);

    // Left toolbar area
    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(4, 4, 4, 4);
    leftLayout->setSpacing(6);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Discovery", "Reading", "Citation", "Sharing"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 120px; }");
    leftLayout->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Source→Target...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    leftLayout->addWidget(inputField_);

    renderBtn_ = new QPushButton("Render");
    renderBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 14px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(renderBtn_, &QPushButton::clicked, this, &PaperSankeyDiagram2::onRender);
    leftLayout->addWidget(renderBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 14px; border: 1px solid #dc2626; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperSankeyDiagram2::onClear);
    leftLayout->addWidget(clearBtn_);

    infoLabel_ = new QLabel("Flows: 0 | Dominant: 0 | Total: 0.0");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    leftLayout->addWidget(infoLabel_);

    leftLayout->addStretch();

    layout->addWidget(leftPanel, 1);

    // Right stretch area (diagram renders in paintEvent)
    auto* rightStretch = new QWidget();
    rightStretch->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    layout->addWidget(rightStretch, 4);

    setMinimumSize(700, 500);
}

void PaperSankeyDiagram2::addEntry(const SankeyEntry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit flowSelected(entry.id, entry.flow);
    update();
}

QList<SankeyEntry> PaperSankeyDiagram2::entries() const { return entries_; }

int PaperSankeyDiagram2::dominantCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.dominant) c++;
    return c;
}

qreal PaperSankeyDiagram2::totalFlow() const {
    qreal t = 0;
    for (const auto& e : entries_) t += e.flow;
    return t;
}

QMap<QString, int> PaperSankeyDiagram2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperSankeyDiagram2::onRender() {
    QString text = inputField_->text().trimmed();

    QStringList sources = {"Search", "Recommend", "Citation", "Feed", "Archive"};
    QStringList targets = {"Abstract", "PDF", "Reference", "Bookmark", "Export"};
    QStringList categories = {"discovery", "reading", "citing", "sharing"};
    QColor palette[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"),
        QColor("#dc2626"), QColor("#7c3aed")
    };

    // Determine source name from input or random
    QString sourceName = text.isEmpty()
        ? sources[QRandomGenerator::global()->bounded(sources.size())]
        : text.section("→", 0, 0).trimmed().isEmpty()
            ? sources[QRandomGenerator::global()->bounded(sources.size())]
            : text.section("→", 0, 0).trimmed();

    // Determine target from input or random
    QString targetHint;
    if (!text.isEmpty() && text.contains("→")) {
        targetHint = text.section("→", 1).trimmed();
    }

    // Generate flows
    int flowCount = 1 + QRandomGenerator::global()->bounded(5);
    qreal maxFlow = 0;

    for (int i = 0; i < flowCount; ++i) {
        SankeyEntry e;
        e.id = entries_.size() + 1;
        e.source = sourceName;
        e.flow = 1.0 + QRandomGenerator::global()->bounded(100);
        e.connections = 1 + QRandomGenerator::global()->bounded(20);
        e.dominant = e.flow > 70.0;
        e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
        e.target = targetHint.isEmpty()
            ? targets[QRandomGenerator::global()->bounded(targets.size())]
            : targetHint;
        e.color = palette[QRandomGenerator::global()->bounded(5)];

        if (e.flow > maxFlow) maxFlow = e.flow;
        entries_.append(e);
    }

    // Compute width proportional to flow / maxFlow
    for (auto& e : entries_) {
        e.width = maxFlow > 0 ? e.flow / maxFlow : 0;
    }

    saveSettings();
    updateInfo();
    if (!entries_.isEmpty()) {
        const auto& last = entries_.last();
        emit flowSelected(last.id, last.flow);
    }
    update();
    inputField_->clear();
}

void PaperSankeyDiagram2::onClear() {
    entries_.clear();
    saveSettings();
    updateInfo();
    update();
}

void PaperSankeyDiagram2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Render a Sankey diagram");
        return;
    }

    int w = width(), h = height();

    // 3-column layout: sankey view, category legend, stats
    int col1End = w * 2 / 5;
    int col2End = w * 3 / 5;
    int col3End = w;

    drawSankeyView(p, QRect(10, 40, col1End - 20, h - 60));
    drawCategoryLegend(p, QRect(col1End + 5, 40, col2End - col1End - 10, h - 60));
    drawStats(p, QRect(col2End + 5, 40, col3End - col2End - 15, h - 60));
}

void PaperSankeyDiagram2::drawSankeyView(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 8, "Sankey Diagram");

    int n = entries_.size();
    if (n == 0) return;

    // Find max flow for proportional sizing
    qreal maxFlow = 1;
    for (const auto& e : entries_) maxFlow = qMax(maxFlow, e.flow);

    // Collect unique sources and targets for bar positioning
    QMap<QString, qreal> sourceFlow, targetFlow;
    for (const auto& e : entries_) {
        sourceFlow[e.source] += e.flow;
        targetFlow[e.target] += e.flow;
    }

    QStringList sourceList = sourceFlow.keys();
    QStringList targetList = targetFlow.keys();
    int nSrc = sourceList.size();
    int nTgt = targetList.size();

    int leftX = rect.x() + 80;
    int rightX = rect.x() + rect.width() - 80;
    int barWidth = 22;
    int drawH = rect.height() - 20;
    int topY = rect.y() + 10;

    // Compute total source flow for spacing
    qreal totalSrcFlow = 0;
    for (const auto& f : sourceFlow) totalSrcFlow += f;
    if (totalSrcFlow == 0) totalSrcFlow = 1;

    // Map source name -> y position of its bar top
    QMap<QString, qreal> srcBarY, srcBarH;
    qreal curY = topY;
    for (const auto& src : sourceList) {
        qreal barH = (sourceFlow[src] / totalSrcFlow) * drawH;
        srcBarY[src] = curY;
        srcBarH[src] = barH;
        curY += barH;
    }

    // Map target name -> y position of its bar top
    qreal totalTgtFlow = 0;
    for (const auto& f : targetFlow) totalTgtFlow += f;
    if (totalTgtFlow == 0) totalTgtFlow = 1;

    QMap<QString, qreal> tgtBarY, tgtBarH;
    curY = topY;
    for (const auto& tgt : targetList) {
        qreal barH = (targetFlow[tgt] / totalTgtFlow) * drawH;
        tgtBarY[tgt] = curY;
        tgtBarH[tgt] = barH;
        curY += barH;
    }

    // Draw source bars (left side)
    for (const auto& src : sourceList) {
        qreal y = srcBarY[src];
        qreal h = srcBarH[src];
        QColor barColor(59, 130, 246);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(QRectF(leftX - barWidth, y, barWidth, h), 4, 4);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRectF(leftX - barWidth - 70, y, 66, h), Qt::AlignRight | Qt::AlignVCenter, src);
    }

    // Draw target bars (right side)
    for (const auto& tgt : targetList) {
        qreal y = tgtBarY[tgt];
        qreal h = tgtBarH[tgt];
        QColor barColor(16, 163, 74);
        p.setPen(Qt::NoPen);
        p.setBrush(barColor);
        p.drawRoundedRect(QRectF(rightX, y, barWidth, h), 4, 4);
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(QRectF(rightX + barWidth + 4, y, 70, h), Qt::AlignLeft | Qt::AlignVCenter, tgt);
    }

    // Track consumed heights per source/target for connection stacking
    QMap<QString, qreal> srcUsed, tgtUsed;
    for (const auto& src : sourceList) srcUsed[src] = srcBarY[src];
    for (const auto& tgt : targetList) tgtUsed[tgt] = tgtBarY[tgt];

    // Draw curved connections between source and target bars
    for (const auto& e : entries_) {
        qreal flowRatio = totalSrcFlow > 0 ? e.flow / totalSrcFlow : 0;
        qreal connH = flowRatio * drawH;
        connH = qMax(connH, 2.0);

        qreal srcY1 = srcUsed[e.source];
        srcUsed[e.source] += connH;
        qreal srcY2 = srcUsed[e.source];

        qreal tgtY1 = tgtUsed[e.target];
        tgtUsed[e.target] += connH;
        qreal tgtY2 = tgtUsed[e.target];

        qreal srcMid = (srcY1 + srcY2) / 2.0;
        qreal tgtMid = (tgtY1 + tgtY2) / 2.0;

        int alpha = e.dominant ? 160 : 90;
        QColor fill(e.color.red(), e.color.green(), e.color.blue(), alpha);

        QPainterPath path;
        path.moveTo(leftX, srcY1);
        path.cubicTo((leftX + rightX) / 2.0, srcY1,
                     (leftX + rightX) / 2.0, tgtY1,
                     rightX, tgtY1);
        path.lineTo(rightX, tgtY2);
        path.cubicTo((leftX + rightX) / 2.0, tgtY2,
                     (leftX + rightX) / 2.0, srcY2,
                     leftX, srcY2);
        path.closeSubpath();

        p.setPen(Qt::NoPen);
        p.setBrush(fill);
        p.drawPath(path);

        // Flow label on dominant connections
        if (e.dominant && connH > 6) {
            p.setPen(QColor(15, 23, 42));
            p.setFont(QFont("Arial", 7));
            p.drawText(QRectF((leftX + rightX) / 2.0 - 20, srcMid - 6, 40, 12),
                       Qt::AlignCenter, QString::number(static_cast<int>(e.flow)));
        }
    }
}

void PaperSankeyDiagram2::drawCategoryLegend(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() - 8, "Legend");

    auto counts = categoryCounts();
    QStringList categories = {"discovery", "reading", "citing", "sharing"};
    QString labels[] = {"Discovery", "Reading", "Citation", "Sharing"};
    QColor colors[] = {
        QColor("#3b82f6"), QColor("#16a34a"), QColor("#d97706"), QColor("#dc2626")
    };

    int itemH = qMin(32, (rect.height() - 20) / 4);
    for (int i = 0; i < 4; ++i) {
        int y = rect.y() + 10 + i * (itemH + 6);

        // Colored square
        p.setPen(Qt::NoPen);
        p.setBrush(colors[i]);
        p.drawRoundedRect(rect.x() + 5, y + 2, 14, 14, 3, 3);

        // Label
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(rect.x() + 24, y - 1, rect.width() / 2 - 24, 20,
                   Qt::AlignVCenter, labels[i]);

        // Count
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x() + rect.width() / 2, y - 1, rect.width() / 2 - 5, 20,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(count) + " flows");
    }
}

void PaperSankeyDiagram2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Flows", QString::number(entries_.size()), QColor("#3b82f6")},
        {"Dominant",    QString::number(dominantCount()), QColor("#16a34a")},
        {"Total Flow",  QString::number(totalFlow(), 'f', 1), QColor("#d97706")}
    };

    int boxH = qMin(48, (rect.height() - 20) / 3);
    for (int i = 0; i < stats.size(); ++i) {
        int y = rect.y() + 10 + i * (boxH + 8);

        // Background box
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(rect.x(), y, rect.width(), boxH, 6, 6);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 14, QFont::Bold));
        p.drawText(QRectF(rect.x() + 10, y + 4, rect.width() - 20, 24),
                   Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRectF(rect.x() + 10, y + 28, rect.width() - 20, 16),
                   Qt::AlignVCenter, stats[i].label);
    }
}

void PaperSankeyDiagram2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Flows: 0 | Dominant: 0 | Total: 0.0");
        return;
    }
    infoLabel_->setText(QString("Flows: %1 | Dominant: %2 | Total: %3")
        .arg(entries_.size())
        .arg(dominantCount())
        .arg(totalFlow(), 0, 'f', 1));
}

void PaperSankeyDiagram2::loadSettings() {
    entries_.clear();
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        SankeyEntry e;
        e.id = settings_.value("id").toInt();
        e.source = settings_.value("source").toString();
        e.target = settings_.value("target").toString();
        e.category = settings_.value("category").toString();
        e.flow = settings_.value("flow").toDouble();
        e.width = settings_.value("width").toDouble();
        e.connections = settings_.value("connections").toInt();
        e.dominant = settings_.value("dominant").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperSankeyDiagram2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("source", entries_[i].source);
        settings_.setValue("target", entries_[i].target);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("flow", entries_[i].flow);
        settings_.setValue("width", entries_[i].width);
        settings_.setValue("connections", entries_[i].connections);
        settings_.setValue("dominant", entries_[i].dominant);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
