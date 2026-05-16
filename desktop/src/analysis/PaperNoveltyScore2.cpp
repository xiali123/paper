#include "analysis/PaperNoveltyScore2.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QtMath>

PaperNoveltyScore2::PaperNoveltyScore2(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "NoveltyScore2")
{
    setupUI();
    loadSettings();

    if (entries_.isEmpty()) {
        QStringList categories = {"Methodology", "Result", "Framework", "Theory", "Application"};
        QStringList papers = {"Attention Transfer", "Graph Transformer", "Zero-Shot CLIP",
                              "Neural Architecture Search", "Meta-Learning Optimizer",
                              "Diffusion Sampling", "Contrastive Pretrain", "Sparse Mixer"};
        QStringList metrics = {"f1", "accuracy", "auc", "bleu", "rouge", "map", "ndcg"};
        QColor catColors[] = {
            QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
            QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
            QColor(0x7c, 0x3a, 0xed)
        };
        for (int i = 0; i < 8; ++i) {
            NoveltyScore2Entry e;
            e.id = i + 1;
            e.paper = papers[i];
            int cIdx = i % 5;
            e.category = categories[cIdx];
            e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
            e.score = 40.0 + QRandomGenerator::global()->bounded(550) / 10.0;
            e.citations = 5 + QRandomGenerator::global()->bounded(200);
            e.novel = e.score >= 70.0;
            e.color = catColors[cIdx];
            addEntry(e);
        }
    }
}

void PaperNoveltyScore2::setupUI() {
    auto* layout = new QVBoxLayout(this);
    auto* toolbar = new QHBoxLayout();

    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Methodology", "Result", "Framework", "Theory", "Application"});
    toolbar->addWidget(new QLabel("Category:"));
    toolbar->addWidget(categoryCombo_, 1);

    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Enter paper name...");
    inputField_->setStyleSheet("QLineEdit { padding: 6px; border: 1px solid #cbd5e1; border-radius: 4px; }");
    toolbar->addWidget(inputField_, 2);

    analyzeBtn_ = new QPushButton("Analyze");
    analyzeBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(analyzeBtn_, &QPushButton::clicked, this, &PaperNoveltyScore2::onAnalyze);
    toolbar->addWidget(analyzeBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperNoveltyScore2::onClear);
    toolbar->addWidget(clearBtn_);

    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Novelty Score Analyzer");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(640, 520);
}

void PaperNoveltyScore2::addEntry(const NoveltyScore2Entry& entry) {
    entries_.append(entry);
    saveSettings();
    updateInfo();
    emit scoreCalculated(entry.id, entry.score);
    update();
}

QList<NoveltyScore2Entry> PaperNoveltyScore2::entries() const {
    return entries_;
}

int PaperNoveltyScore2::novelCount() const {
    int c = 0;
    for (const auto& e : entries_)
        if (e.novel) c++;
    return c;
}

qreal PaperNoveltyScore2::avgScore() const {
    if (entries_.isEmpty()) return 0.0;
    qreal sum = 0.0;
    for (const auto& e : entries_) sum += e.score;
    return sum / entries_.size();
}

QMap<QString, int> PaperNoveltyScore2::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& e : entries_) counts[e.category]++;
    return counts;
}

void PaperNoveltyScore2::onAnalyze() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;

    QStringList categories = {"Methodology", "Result", "Framework", "Theory", "Application"};
    QStringList metrics = {"f1", "accuracy", "auc", "bleu", "rouge", "map", "ndcg"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int cIdx = categoryCombo_->currentIndex();
    int count = 1 + QRandomGenerator::global()->bounded(3);
    for (int i = 0; i < count; ++i) {
        NoveltyScore2Entry e;
        e.id = entries_.size() + 1;
        e.paper = text.left(20) + (count > 1 ? QString(" v%1").arg(i + 1) : "");
        int chosenCat = cIdx == 0 ? QRandomGenerator::global()->bounded(5) : (cIdx - 1);
        e.category = categories[chosenCat];
        e.metric = metrics[QRandomGenerator::global()->bounded(metrics.size())];
        e.score = 30.0 + QRandomGenerator::global()->bounded(700) / 10.0;
        e.citations = 1 + QRandomGenerator::global()->bounded(300);
        e.novel = e.score >= 70.0;
        e.color = catColors[chosenCat];
        addEntry(e);
    }
    inputField_->clear();
}

void PaperNoveltyScore2::onClear() {
    entries_.clear();
    saveSettings();
    infoLabel_->setText("Novelty Score Analyzer");
    update();
}

void PaperNoveltyScore2::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (entries_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "No novelty entries - click Analyze");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Novelty Score Analysis");

    int w = width(), h = height();
    int halfW = w / 2 - 20;

    drawScoreView(p, QRect(20, 50, halfW, h - 80));
    drawCategoryChart(p, QRect(w / 2 + 10, 50, halfW, h / 2 - 30));
    drawStats(p, QRect(w / 2 + 10, h / 2 + 20, halfW, h / 2 - 50));
}

void PaperNoveltyScore2::drawScoreView(QPainter& p, const QRect& rect) {
    int show = qMin(10, entries_.size());
    int itemH = qMin(36, (rect.height() - 10) / qMax(show, 1));

    for (int i = 0; i < show; ++i) {
        const auto& e = entries_[i];
        int y = rect.y() + i * (itemH + 4);

        // Card background
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(rect.x(), y, rect.width(), itemH, 6, 6);

        // Left accent bar
        p.setPen(Qt::NoPen);
        p.setBrush(e.color);
        p.drawRoundedRect(rect.x(), y, 4, itemH, 2, 2);

        // Score bar (background track)
        int barX = rect.x() + 8;
        int barTrackW = rect.width() / 3;
        int barY = y + itemH / 2 - 3;
        p.setBrush(QColor(226, 232, 240));
        p.drawRoundedRect(barX, barY, barTrackW, 6, 3, 3);

        // Score bar (filled)
        qreal scoreRatio = qBound(0.0, e.score / 100.0, 1.0);
        int barFillW = static_cast<int>(barTrackW * scoreRatio);
        p.setBrush(e.novel ? QColor(0x16, 0xa3, 0x4a) : QColor(0xd9, 0x77, 0x06));
        p.drawRoundedRect(barX, barY, barFillW, 6, 3, 3);

        // Paper name
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + barTrackW + 16, y + 2, rect.width() - barTrackW - 24, 16,
                   Qt::AlignVCenter,
                   e.paper.left(16) + (e.novel ? " [NOVEL]" : ""));

        // Metric + category line
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + barTrackW + 16, y + 18, rect.width() - barTrackW - 24, 14,
                   Qt::AlignVCenter,
                   e.metric + " | " + e.category);

        // Score value
        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8, QFont::Bold));
        p.drawText(rect.x() + rect.width() - 50, y + 2, 44, 16,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.score, 'f', 1));

        // Citation count
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 50, y + 18, 44, 14,
                   Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(e.citations) + " cites");
    }
}

void PaperNoveltyScore2::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Category Distribution");

    auto counts = categoryCounts();
    QStringList categories = {"Methodology", "Result", "Framework", "Theory", "Application"};
    QColor catColors[] = {
        QColor(0x3b, 0x82, 0xf6), QColor(0x16, 0xa3, 0x4a),
        QColor(0xd9, 0x77, 0x06), QColor(0xdc, 0x26, 0x26),
        QColor(0x7c, 0x3a, 0xed)
    };

    int total = entries_.size();
    if (total == 0) return;

    // Donut chart center and radius
    int chartSize = qMin(rect.width(), rect.height() - 40);
    int cx = rect.x() + rect.width() / 2;
    int cy = rect.y() + 30 + chartSize / 2;
    int outerR = chartSize / 2 - 4;
    int innerR = outerR * 55 / 100;

    qreal startAngle = 0.0;
    for (int i = 0; i < 5; ++i) {
        int count = counts.contains(categories[i]) ? counts[categories[i]] : 0;
        if (count == 0) continue;
        qreal span = (static_cast<qreal>(count) / total) * 360.0;

        p.setPen(Qt::NoPen);
        p.setBrush(catColors[i]);
        p.drawPie(cx - outerR, cy - outerR, outerR * 2, outerR * 2,
                  static_cast<int>(startAngle * 16), static_cast<int>(span * 16));

        startAngle += span;
    }

    // Inner circle (donut hole)
    p.setBrush(Qt::white);
    p.drawEllipse(cx - innerR, cy - innerR, innerR * 2, innerR * 2);

    // Center text
    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 14, QFont::Bold));
    p.drawText(cx - innerR, cy - 12, innerR * 2, 24,
               Qt::AlignCenter, QString::number(total));
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 8));
    p.drawText(cx - innerR, cy + 8, innerR * 2, 16,
               Qt::AlignCenter, "papers");
}

void PaperNoveltyScore2::drawStats(QPainter& p, const QRect& rect) {
    struct Stat { QString label; QString value; QColor color; };
    QList<Stat> stats = {
        {"Total Papers", QString::number(entries_.size()), QColor(0x3b, 0x82, 0xf6)},
        {"Novel Papers", QString::number(novelCount()), QColor(0x16, 0xa3, 0x4a)},
        {"Avg Score", QString::number(avgScore(), 'f', 1), QColor(0xd9, 0x77, 0x06)},
        {"Avg Citations", QString::number(entries_.isEmpty() ? 0.0
            : std::accumulate(entries_.begin(), entries_.end(), 0.0,
                [](qreal s, const NoveltyScore2Entry& e) { return s + e.citations; })
                / entries_.size(), 'f', 0),
            QColor(0xdc, 0x26, 0x26)}
    };

    int cols = 2;
    int rows = 2;
    int gapX = 8, gapY = 8;
    int boxW = (rect.width() - gapX * (cols - 1)) / cols;
    int boxH = qMin(56, (rect.height() - gapY * (rows - 1)) / rows);

    for (int i = 0; i < stats.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        int bx = rect.x() + col * (boxW + gapX);
        int by = rect.y() + row * (boxH + gapY);

        // Box background
        p.setPen(Qt::NoPen);
        p.setBrush(stats[i].color.lighter(190));
        p.drawRoundedRect(bx, by, boxW, boxH, 8, 8);

        // Top accent line
        p.setBrush(stats[i].color);
        p.drawRoundedRect(bx, by, boxW, 3, 2, 2);

        // Value
        p.setPen(stats[i].color);
        p.setFont(QFont("Arial", 16, QFont::Bold));
        p.drawText(bx + 10, by + 8, boxW - 20, 28, Qt::AlignVCenter, stats[i].value);

        // Label
        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 10, by + 34, boxW - 20, 16, Qt::AlignVCenter, stats[i].label);
    }
}

void PaperNoveltyScore2::updateInfo() {
    if (entries_.isEmpty()) {
        infoLabel_->setText("Novelty Score Analyzer");
        return;
    }
    int avgCites = 0;
    for (const auto& e : entries_) avgCites += e.citations;
    avgCites = entries_.isEmpty() ? 0 : avgCites / entries_.size();
    infoLabel_->setText(QString("%1 papers | %2 novel | avg score: %3 | avg cites: %4")
        .arg(entries_.size())
        .arg(novelCount())
        .arg(avgScore(), 0, 'f', 1)
        .arg(avgCites));
}

void PaperNoveltyScore2::loadSettings() {
    int size = settings_.beginReadArray("entries");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        NoveltyScore2Entry e;
        e.id = settings_.value("id").toInt();
        e.paper = settings_.value("paper").toString();
        e.category = settings_.value("category").toString();
        e.metric = settings_.value("metric").toString();
        e.score = settings_.value("score").toDouble();
        e.citations = settings_.value("citations").toInt();
        e.novel = settings_.value("novel").toBool();
        e.color = QColor(settings_.value("color").toString());
        entries_.append(e);
    }
    settings_.endArray();
    updateInfo();
}

void PaperNoveltyScore2::saveSettings() {
    settings_.beginWriteArray("entries");
    for (int i = 0; i < entries_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", entries_[i].id);
        settings_.setValue("paper", entries_[i].paper);
        settings_.setValue("category", entries_[i].category);
        settings_.setValue("metric", entries_[i].metric);
        settings_.setValue("score", entries_[i].score);
        settings_.setValue("citations", entries_[i].citations);
        settings_.setValue("novel", entries_[i].novel);
        settings_.setValue("color", entries_[i].color.name());
    }
    settings_.endArray();
}
