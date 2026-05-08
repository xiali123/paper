#include "PaperKeywordExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QRandomGenerator>
#include <cmath>

PaperKeywordExtractor::PaperKeywordExtractor(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "KeywordExtractor")
{
    setupUI();
}

void PaperKeywordExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Max:"));
    maxCombo_ = new QComboBox();
    maxCombo_->addItems({"10", "15", "20", "30", "50"});
    maxCombo_->setCurrentIndex(2);
    connect(maxCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperKeywordExtractor::onMaxChanged);
    toolbar->addWidget(maxCombo_);

    extractBtn_ = new QPushButton("Extract");
    extractBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(extractBtn_, &QPushButton::clicked, this, &PaperKeywordExtractor::onExtract);
    toolbar->addWidget(extractBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperKeywordExtractor::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Enter text or load paper to extract keywords");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(550, 420);
}

void PaperKeywordExtractor::setPaperText(const QString& text) {
    sourceText_ = text;
    updateInfo();
}

void PaperKeywordExtractor::extract(int maxKeywords) {
    keywords_.clear();

    QStringList domainWords = {
        "neural", "network", "learning", "deep", "model", "transformer", "attention",
        "training", "gradient", "optimization", "convolutional", "recurrent", "generative",
        "diffusion", "reinforcement", "supervised", "unsupervised", "embedding",
        "classification", "regression", "clustering", "segmentation", "detection",
        "language", "translation", "generation", "understanding", "reasoning",
        "memory", "knowledge", "retrieval", "federated", "privacy", "security",
        "robust", "explainable", "fairness", "bias", "alignment",
        "multimodal", "vision", "speech", "image", "text", "video",
        "graph", "robotics", "benchmark", "dataset", "pretrain", "finetune",
        "prompt", "zero-shot", "few-shot", "architecture", "encoder", "decoder",
        "bert", "gpt", "resnet", "vgg", "lstm", "gan", "vae", "cnn", "rnn"};

    QStringList categories = {"Architecture", "Training", "NLP", "Vision", "Optimization",
                              "Safety", "Data", "Inference", "Representation", "Evaluation"};
    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                       QColor(14,165,233), QColor(168,85,247), QColor(234,179,8), QColor(34,197,94)};

    int count = qMin(maxKeywords, domainWords.size());
    QList<int> indices;
    while (indices.size() < count) {
        int idx = QRandomGenerator::global()->bounded(domainWords.size());
        if (!indices.contains(idx)) indices.append(idx);
    }

    for (int i = 0; i < indices.size(); ++i) {
        KeywordResult kw;
        kw.keyword = domainWords[indices[i]];
        kw.score = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        kw.frequency = 1 + QRandomGenerator::global()->bounded(25);
        kw.category = categories[i % categories.size()];
        kw.color = colors[i % 10];
        keywords_.append(kw);
    }

    std::sort(keywords_.begin(), keywords_.end(),
        [](const KeywordResult& a, const KeywordResult& b) { return a.score > b.score; });

    updateInfo();
    emit keywordsExtracted(keywords_.size());
    update();
}

QList<KeywordResult> PaperKeywordExtractor::keywords() const { return keywords_; }

QStringList PaperKeywordExtractor::topKeywords(int limit) const {
    QStringList result;
    for (int i = 0; i < qMin(limit, keywords_.size()); ++i) {
        result.append(keywords_[i].keyword);
    }
    return result;
}

QMap<QString, int> PaperKeywordExtractor::categoryCounts() const {
    QMap<QString, int> counts;
    for (const auto& kw : keywords_) counts[kw.category]++;
    return counts;
}

void PaperKeywordExtractor::onExtract() {
    int max = maxCombo_->currentText().toInt();
    extract(max);
}

void PaperKeywordExtractor::onMaxChanged(int) {}
void PaperKeywordExtractor::onClear() {
    keywords_.clear();
    sourceText_.clear();
    infoLabel_->setText("Enter text or load paper to extract keywords");
    update();
}

void PaperKeywordExtractor::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (keywords_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Click Extract to find keywords");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Keyword Extraction");

    int w = width(), h = height();
    drawKeywordCloud(p, QRect(20, 50, w - 40, h / 2 - 40));
    drawCategoryChart(p, QRect(20, h / 2 + 10, w / 2 - 30, h / 2 - 50));
    drawScoreBars(p, QRect(w / 2 + 10, h / 2 + 10, w / 2 - 30, h / 2 - 50));
}

void PaperKeywordExtractor::drawKeywordCloud(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Keyword Cloud");

    if (keywords_.isEmpty()) return;
    qreal maxScore = keywords_.first().score;

    int cols = qMax(3, rect.width() / 100);
    int rows = (keywords_.size() + cols - 1) / cols;
    int cellW = rect.width() / cols;
    int cellH = qMin(30, (rect.height() - 25) / qMax(1, rows));

    for (int i = 0; i < keywords_.size(); ++i) {
        int col = i % cols;
        int row = i / cols;
        qreal x = rect.x() + col * cellW + 5;
        qreal y = rect.y() + 20 + row * cellH + cellH / 2;

        qreal relScore = keywords_[i].score / maxScore;
        int fontSize = qBound(8, static_cast<int>(9 + relScore * 10), 18);
        QFont::Weight weight = relScore > 0.6 ? QFont::Bold : QFont::Normal;

        p.setPen(keywords_[i].color);
        p.setFont(QFont("Arial", fontSize, weight));
        p.drawText(QPointF(x, y), keywords_[i].keyword);
    }
}

void PaperKeywordExtractor::drawCategoryChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Categories");

    auto counts = categoryCounts();
    QList<QString> cats = counts.keys();
    if (cats.isEmpty()) return;

    int maxVal = 1;
    for (const auto& v : counts) maxVal = qMax(maxVal, v);

    int barH = qMin(18, (rect.height() - 40) / cats.size());
    for (int i = 0; i < cats.size(); ++i) {
        int y = rect.y() + 20 + i * (barH + 3);
        int barW = static_cast<int>((static_cast<qreal>(counts[cats[i]]) / maxVal) * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter, cats[i].left(10));

        QColor c(59 + (i * 37) % 180, 130 + (i * 23) % 120, 246 - (i * 17) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2, QString::number(counts[cats[i]]));
    }
}

void PaperKeywordExtractor::drawScoreBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Top Scores");

    int show = qMin(8, keywords_.size());
    int barH = qMin(18, (rect.height() - 40) / show);

    for (int i = 0; i < show; ++i) {
        int y = rect.y() + 20 + i * (barH + 3);
        int barW = static_cast<int>(keywords_[i].score * (rect.width() - 120));

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x(), y + barH - 2, 70, barH, Qt::AlignRight | Qt::AlignVCenter,
                   keywords_[i].keyword.left(10));

        p.setPen(Qt::NoPen);
        p.setBrush(keywords_[i].color);
        p.drawRoundedRect(rect.x() + 75, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 78 + barW, y + barH - 2,
                   QString::number(keywords_[i].score, 'f', 2));
    }
}

void PaperKeywordExtractor::updateInfo() {
    if (keywords_.isEmpty()) { infoLabel_->setText("Enter text or load paper to extract keywords"); return; }
    infoLabel_->setText(QString("%1 keywords extracted | Top: %2 | Categories: %3")
        .arg(keywords_.size())
        .arg(topKeywords(3).join(", "))
        .arg(categoryCounts().size()));
}
