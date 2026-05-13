#include "tools/PaperPublicationPipeline2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperPublicationPipeline2::PaperPublicationPipeline2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PublicationPipeline2")
{
    setupUI();
    loadSettings();
}

void PaperPublicationPipeline2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    auto* toolbar = new QHBoxLayout();
    toolbar->setSpacing(8);

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Journal", "Conference", "Workshop", "Preprint", "Book Chapter"});
    categoryCombo_->setStyleSheet(
        "QComboBox { padding: 5px 10px; border: 1px solid #cbd5e1; border-radius: 4px; "
        "background: white; min-width: 120px; }"
        "QComboBox::drop-down { border: none; }");
    toolbar->addWidget(categoryCombo_);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper title to track...");
    inputField_->setStyleSheet(
        "QLineEdit { padding: 6px 10px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 1);

    trackBtn_ = new QPushButton("Track");
    trackBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 5px 16px; "
        "border-radius: 4px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }");
    connect(trackBtn_, &QPushButton::clicked, this, &PaperPublicationPipeline2::onTrack);
    toolbar->addWidget(trackBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet(
        "QPushButton { color: #dc2626; padding: 5px 12px; border: 1px solid #fca5a5; "
        "border-radius: 4px; background: white; }"
        "QPushButton:hover { background: #fef2f2; }");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperPublicationPipeline2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Track papers through the publication pipeline");
    infoLabel_->setStyleSheet("font-size: 12px; color: #64748b; padding: 2px 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(720, 560);
}

void PaperPublicationPipeline2::addEntry(const PublicationPipeline2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit stageChanged(entry.id, entry.progress);
    update();
}

QList<PublicationPipeline2Entry> PaperPublicationPipeline2::entries() const {
    return entries_;
}

int PaperPublicationPipeline2::acceptedCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.accepted) ++c;
    return c;
}

qreal PaperPublicationPipeline2::avgProgress() const {
    if (entries_.isEmpty()) return 0.0;
    qreal total = 0;
    for (const auto& e : entries_) total += e.progress;
    return total / entries_.size();
}

QMap<QString, int> PaperPublicationPipeline2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperPublicationPipeline2::onTrack() {
    static const QStringList categories = {"Journal", "Conference", "Workshop", "Preprint", "Book Chapter"};
    static const QStringList stages = {"Draft", "Submitted", "Under Review", "Revision", "Accepted", "Published"};
    static const QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };

    QString paperTitle = inputField_->text().trimmed();
    if (paperTitle.isEmpty()) return;

    PublicationPipeline2Entry e;
    e.id = entries_.isEmpty() ? 1 : entries_.last().id + 1;
    e.paper = paperTitle;
    e.category = categories[QRandomGenerator::global()->bounded(categories.size())];
    e.stage = stages[QRandomGenerator::global()->bounded(stages.size())];
    e.progress = static_cast<qreal>(QRandomGenerator::global()->bounded(101));
    e.reviews = QRandomGenerator::global()->bounded(6);
    e.accepted = (e.stage == "Accepted" || e.stage == "Published");
    e.color = palette[QRandomGenerator::global()->bounded(palette.size())];

    entries_.append(e);
    saveSettings();
    updateInfo();
    emit stageChanged(e.id, e.progress);
    update();
    inputField_->clear();
}

void PaperPublicationPipeline2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Track papers through the publication pipeline");
    update();
}

void PaperPublicationPipeline2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), QColor(0xf8fafc));

    if (entries_.isEmpty()) {
        p.setPen(QColor(0x94a3b8));
        p.setFont(QFont("Arial", 13));
        p.drawText(rect(), Qt::AlignCenter, "No papers tracked -- add a paper to get started");
        return;
    }

    p.setPen(QColor(0x0f172a));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(20, 30, "Publication Pipeline Tracker");

    int w = width(), h = height();
    drawStats(p, QRect(20, 44, w - 40, 70));
    drawPipelineView(p, QRect(10, 124, w - 20, h / 2 + 10));
    drawCategoryChart(p, QRect(10, h / 2 + 144, w - 20, h / 2 - 164));
}

void PaperPublicationPipeline2::drawPipelineView(QPainter& p, const QRect& rect) {
    static const QStringList stages = {"Draft", "Submitted", "Under Review", "Revision", "Accepted", "Published"};
    static const QMap<QString, QColor> stageColors = {
        {"Draft",        QColor(0x94a3b8)},
        {"Submitted",    QColor(0x3b82f6)},
        {"Under Review", QColor(0xd97706)},
        {"Revision",     QColor(0xdc2626)},
        {"Accepted",     QColor(0x16a34a)},
        {"Published",    QColor(0x7c3aed)}
    };

    int stageCount = stages.size();
    int margin = 30;
    int nodeW = qMin(100, (rect.width() - 2 * margin - (stageCount - 1) * 40) / stageCount);
    int nodeH = 36;
    int gap = 40;
    int totalW = stageCount * nodeW + (stageCount - 1) * gap;
    int startX = rect.x() + margin + qMax(0, (rect.width() - 2 * margin - totalW) / 2);
    int nodeY = rect.y() + 10;

    // Draw connecting lines between stage nodes
    for (int i = 0; i < stageCount - 1; ++i) {
        int x1 = startX + i * (nodeW + gap) + nodeW;
        int x2 = startX + (i + 1) * (nodeW + gap);
        int ly = nodeY + nodeH / 2;
        p.setPen(QPen(QColor(0xcbd5e1), 2));
        p.drawLine(x1 + 2, ly, x2 - 2, ly);
        // Arrow head
        p.setBrush(QColor(0xcbd5e1));
        p.setPen(Qt::NoPen);
        QPolygon arrow;
        arrow << QPoint(x2 - 2, ly) << QPoint(x2 - 9, ly - 4) << QPoint(x2 - 9, ly + 4);
        p.drawPolygon(arrow);
    }

    // Draw stage nodes
    for (int i = 0; i < stageCount; ++i) {
        int x = startX + i * (nodeW + gap);
        QColor sc = stageColors[stages[i]];

        p.setPen(Qt::NoPen);
        p.setBrush(sc);
        p.drawRoundedRect(x, nodeY, nodeW, nodeH, 6, 6);

        p.setPen(Qt::white);
        p.setFont(QFont("Arial", 9, QFont::Bold));
        p.drawText(QRect(x, nodeY, nodeW, nodeH), Qt::AlignCenter, stages[i]);
    }

    // Group papers by their current stage
    QMap<QString, QList<int>> stagePapers;
    for (int i = 0; i < entries_.size(); ++i)
        stagePapers[entries_[i].stage].append(i);

    // Draw paper cards below each stage node
    int cardW = qMin(96, nodeW + 10);
    int cardH = 52;
    int cardStartY = nodeY + nodeH + 14;

    for (int i = 0; i < stageCount; ++i) {
        int nodeCenterX = startX + i * (nodeW + gap) + nodeW / 2;
        const QList<int>& paperIndices = stagePapers[stages[i]];

        for (int j = 0; j < paperIndices.size() && j < 4; ++j) {
            const auto& e = entries_[paperIndices[j]];
            int cx = nodeCenterX - cardW / 2;
            int cy = cardStartY + j * (cardH + 6);

            // Card background
            p.setPen(Qt::NoPen);
            p.setBrush(Qt::white);
            p.drawRoundedRect(cx, cy, cardW, cardH, 5, 5);

            // Card left color accent
            p.setBrush(e.color);
            p.drawRoundedRect(cx, cy, 4, cardH, 2, 2);

            // Progress fill at bottom
            qreal fillRatio = qBound(0.0, e.progress / 100.0, 1.0);
            int fillW = static_cast<int>(fillRatio * (cardW - 8));
            p.setBrush(e.color.lighter(160));
            p.setOpacity(0.5);
            p.drawRoundedRect(cx + 4, cy + cardH - 6, fillW, 4, 2, 2);
            p.setOpacity(1.0);

            // Paper name
            p.setPen(QColor(0x1e293b));
            p.setFont(QFont("Arial", 8, QFont::Bold));
            QString name = e.paper;
            if (p.fontMetrics().horizontalAdvance(name) > cardW - 16)
                name = p.fontMetrics().elidedText(name, Qt::ElideRight, cardW - 16);
            p.drawText(cx + 8, cy + 4, cardW - 16, 16, Qt::AlignLeft | Qt::AlignVCenter, name);

            // Category + progress
            p.setPen(QColor(0x64748b));
            p.setFont(QFont("Arial", 7));
            p.drawText(cx + 8, cy + 18, cardW - 16, 12, Qt::AlignLeft | Qt::AlignVCenter,
                QString("%1 | %2%").arg(e.category).arg(static_cast<int>(e.progress)));

            // Review count
            p.setPen(QColor(0xd97706));
            p.setFont(QFont("Arial", 7));
            p.drawText(cx + 8, cy + 30, cardW - 16, 12, Qt::AlignLeft | Qt::AlignVCenter,
                QString("Reviews: %1").arg(e.reviews));

            // Accepted badge
            if (e.accepted) {
                int badgeW = 50;
                int badgeH = 14;
                int bx = cx + cardW - badgeW - 4;
                int by = cy + 4;
                p.setPen(Qt::NoPen);
                p.setBrush(QColor(0x16a34a));
                p.drawRoundedRect(bx, by, badgeW, badgeH, 3, 3);
                p.setPen(Qt::white);
                p.setFont(QFont("Arial", 7, QFont::Bold));
                p.drawText(QRect(bx, by, badgeW, badgeH), Qt::AlignCenter, "Accepted");
            }
        }

        // Overflow indicator
        if (paperIndices.size() > 4) {
            int cy = cardStartY + 4 * (cardH + 6);
            p.setPen(QColor(0x94a3b8));
            p.setFont(QFont("Arial", 7));
            p.drawText(QRect(nodeCenterX - cardW / 2, cy, cardW, 14),
                Qt::AlignCenter, QString("+%1 more").arg(paperIndices.size() - 4));
        }
    }
}

void PaperPublicationPipeline2::drawCategoryChart(QPainter& p, const QRect& rect) {
    // Title
    p.setPen(QColor(0x334155));
    p.setFont(QFont("Arial", 11, QFont::Bold));
    p.drawText(rect.x(), rect.y() + 14, "Stage Distribution by Category");

    static const QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
        QColor(0xdc2626), QColor(0x7c3aed)
    };
    static const QStringList stages = {"Draft", "Submitted", "Under Review", "Revision", "Accepted", "Published"};

    // Compute stage distribution
    QMap<QString, int> stageCounts;
    for (const auto& e : entries_) stageCounts[e.stage]++;
    int maxCount = 1;
    for (const auto& s : stages)
        maxCount = qMax(maxCount, stageCounts.value(s, 0));

    int barH = 18;
    int barGap = 6;
    int labelW = 80;
    int chartX = rect.x() + labelW;
    int chartW = rect.width() - labelW - 50;
    int y = rect.y() + 28;

    for (int i = 0; i < stages.size(); ++i) {
        int count = stageCounts.value(stages[i], 0);
        QColor c = palette[i % palette.size()];

        // Stage label
        p.setPen(QColor(0x475569));
        p.setFont(QFont("Arial", 9));
        p.drawText(rect.x(), y, labelW - 6, barH, Qt::AlignRight | Qt::AlignVCenter, stages[i]);

        // Bar background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0xf1f5f9));
        p.drawRoundedRect(chartX, y, chartW, barH, 3, 3);

        // Filled bar
        if (count > 0) {
            int fillW = static_cast<int>((static_cast<qreal>(count) / maxCount) * chartW);
            p.setBrush(c);
            p.drawRoundedRect(chartX, y, fillW, barH, 3, 3);
        }

        // Count label
        p.setPen(QColor(0x1e293b));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(chartX + chartW + 6, y, 36, barH, Qt::AlignLeft | Qt::AlignVCenter,
            QString::number(count));

        y += barH + barGap;
    }
}

void PaperPublicationPipeline2::drawStats(QPainter& p, const QRect& rect) {
    static const QList<QColor> palette = {
        QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706), QColor(0x7c3aed)
    };

    struct Stat { QString label; QString value; };
    int totalReviews = 0;
    for (const auto& e : entries_) totalReviews += e.reviews;

    QList<Stat> stats = {
        {"Total Papers",  QString::number(entries_.size())},
        {"Accepted",      QString::number(acceptedCount())},
        {"Avg Progress",  QString::number(avgProgress(), 'f', 1) + "%"},
        {"Total Reviews", QString::number(totalReviews)}
    };

    int boxCount = stats.size();
    int gap = 12;
    int boxW = (rect.width() - (boxCount - 1) * gap) / boxCount;
    int boxH = rect.height();

    for (int i = 0; i < boxCount; ++i) {
        int x = rect.x() + i * (boxW + gap);
        int y = rect.y();
        QColor c = palette[i];

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(c.lighter(190));
        p.drawRoundedRect(x, y, boxW, boxH, 8, 8);

        // Top accent line
        p.setBrush(c);
        p.drawRoundedRect(x, y, boxW, 4, 2, 2);

        // Value
        p.setPen(c);
        p.setFont(QFont("Arial", 18, QFont::Bold));
        p.drawText(QRect(x, y + 10, boxW, 30), Qt::AlignCenter, stats[i].value);

        // Label
        p.setPen(QColor(0x64748b));
        p.setFont(QFont("Arial", 9));
        p.drawText(QRect(x, y + 42, boxW, 18), Qt::AlignCenter, stats[i].label);
    }
}

void PaperPublicationPipeline2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Track papers through the publication pipeline");
        return;
    }
    infoLabel_->setText(QString("Papers: %1 | Accepted: %2 | Avg Progress: %3% | Reviews: %4")
        .arg(entries_.size())
        .arg(acceptedCount())
        .arg(QString::number(avgProgress(), 'f', 1))
        .arg([this]() { int t = 0; for (const auto& e : entries_) t += e.reviews; return t; }()));
}

void PaperPublicationPipeline2::loadSettings() {
    settings_.beginGroup("PublicationPipeline2");
    int count = settings_.value("count", 0).toInt();
    for (int i = 0; i < count; ++i) {
        PublicationPipeline2Entry e;
        e.id = settings_.value(QString("id_%1").arg(i)).toInt();
        e.paper = settings_.value(QString("paper_%1").arg(i)).toString();
        e.category = settings_.value(QString("category_%1").arg(i)).toString();
        e.stage = settings_.value(QString("stage_%1").arg(i)).toString();
        e.progress = settings_.value(QString("progress_%1").arg(i)).toDouble();
        e.reviews = settings_.value(QString("reviews_%1").arg(i)).toInt();
        e.accepted = settings_.value(QString("accepted_%1").arg(i)).toBool();
        e.color = QColor(settings_.value(QString("color_%1").arg(i)).toString());
        entries_.append(e);
    }
    settings_.endGroup();

    // Seed 8 entries if empty
    if (entries_.isEmpty()) {
        static const QStringList categories = {"Journal", "Conference", "Workshop", "Preprint", "Book Chapter"};
        static const QStringList stages = {"Draft", "Submitted", "Under Review", "Revision", "Accepted", "Published"};
        static const QList<QColor> palette = {
            QColor(0x3b82f6), QColor(0x16a34a), QColor(0xd97706),
            QColor(0xdc2626), QColor(0x7c3aed)
        };

        struct Seed { QString paper; QString category; QString stage; qreal progress; int reviews; };
        QList<Seed> seeds = {
            {"Deep Learning for NLP",           "Journal",       "Published",     100.0, 4},
            {"Graph Neural Networks Survey",     "Conference",    "Accepted",       92.0, 3},
            {"Federated Learning Privacy",       "Workshop",      "Under Review",   65.0, 2},
            {"Transformer Efficiency Methods",   "Conference",    "Revision",       78.0, 3},
            {"Quantum ML Foundations",           "Preprint",      "Submitted",      35.0, 0},
            {"Multi-Modal Reasoning",            "Journal",       "Under Review",   55.0, 2},
            {"Edge AI Optimization",             "Workshop",      "Draft",          12.0, 0},
            {"AutoML for Time Series",           "Book Chapter",  "Revision",       82.0, 5}
        };

        for (int i = 0; i < seeds.size(); ++i) {
            PublicationPipeline2Entry e;
            e.id = i + 1;
            e.paper = seeds[i].paper;
            e.category = seeds[i].category;
            e.stage = seeds[i].stage;
            e.progress = seeds[i].progress;
            e.reviews = seeds[i].reviews;
            e.accepted = (seeds[i].stage == "Accepted" || seeds[i].stage == "Published");
            e.color = palette[i % palette.size()];
            entries_.append(e);
        }
        saveSettings();
    }

    updateInfo();
}

void PaperPublicationPipeline2::saveSettings() {
    settings_.beginGroup("PublicationPipeline2");
    settings_.remove("");
    settings_.setValue("count", entries_.size());
    for (int i = 0; i < entries_.size(); ++i) {
        const auto& e = entries_[i];
        settings_.setValue(QString("id_%1").arg(i), e.id);
        settings_.setValue(QString("paper_%1").arg(i), e.paper);
        settings_.setValue(QString("category_%1").arg(i), e.category);
        settings_.setValue(QString("stage_%1").arg(i), e.stage);
        settings_.setValue(QString("progress_%1").arg(i), e.progress);
        settings_.setValue(QString("reviews_%1").arg(i), e.reviews);
        settings_.setValue(QString("accepted_%1").arg(i), e.accepted);
        settings_.setValue(QString("color_%1").arg(i), e.color.name());
    }
    settings_.endGroup();
}
