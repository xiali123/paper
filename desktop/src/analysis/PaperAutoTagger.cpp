#include "analysis/PaperAutoTagger.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QRandomGenerator>

PaperAutoTagger::PaperAutoTagger(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperAutoTagger::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Min Confidence:"));
    confidenceCombo_ = new QComboBox();
    confidenceCombo_->addItems({"0.5", "0.6", "0.7", "0.8", "0.9"});
    confidenceCombo_->setCurrentIndex(2);
    connect(confidenceCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperAutoTagger::onConfidenceChanged);
    toolbar->addWidget(confidenceCombo_);

    autoTagBtn_ = new QPushButton("Auto Tag");
    autoTagBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(autoTagBtn_, &QPushButton::clicked, this, &PaperAutoTagger::onAutoTag);
    toolbar->addWidget(autoTagBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperAutoTagger::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Auto-tag papers with suggested labels");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    setMinimumSize(580, 460);
}

void PaperAutoTagger::setPapers(const QList<QPair<int, QString>>& papers) {
    Q_UNUSED(papers);
}

void PaperAutoTagger::autoTag() {
    results_.clear();

    QStringList tagPool = {
        "deep-learning", "nlp", "computer-vision", "transformer", "attention",
        "generative", "reinforcement-learning", "graph-neural-network", "optimization",
        "classification", "segmentation", "object-detection", "language-model",
        "multimodal", "self-supervised", "few-shot", "transfer-learning",
        "robustness", "explainability", "fairness", "privacy", "federated",
        "efficient", "real-time", "pre-training", "fine-tuning", "diffusion",
        "gan", "vae", "bert", "gpt", "resnet", "cnn", "rnn", "lstm"};

    QStringList titles = {
        "Deep Learning for Image Recognition", "BERT: Pre-training of Deep Bidirectional Transformers",
        "Attention Is All You Need", "Generative Adversarial Networks",
        "ResNet: Deep Residual Learning", "YOLO: Real-Time Object Detection",
        "Transformer-XL: Language Modeling", "GPT-3: Language Models are Few-Shot Learners",
        "DALL-E: Zero-Shot Text-to-Image Generation", "CLIP: Contrastive Learning",
        "ViT: Vision Transformer", "EfficientNet: Rethinking Model Scaling",
        "Diffusion Models Beat GANs", "LLaMA: Open Foundation Language Models",
        "InstructGPT: Training language models to follow instructions"};

    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153)};

    for (int i = 0; i < titles.size(); ++i) {
        AutoTagResult r;
        r.paperId = i + 1;
        r.paperTitle = titles[i];

        int numTags = 3 + QRandomGenerator::global()->bounded(5);
        QList<int> indices;
        while (indices.size() < numTags) {
            int idx = QRandomGenerator::global()->bounded(tagPool.size());
            if (!indices.contains(idx)) indices.append(idx);
        }

        qreal minConf = confidenceCombo_->currentText().toDouble();
        for (int idx : indices) {
            qreal conf = 0.4 + QRandomGenerator::global()->bounded(60) / 100.0;
            if (conf >= minConf) {
                r.suggestedTags.append(tagPool[idx]);
            }
        }

        r.confidence = r.suggestedTags.isEmpty() ? 0 : 0.6 + QRandomGenerator::global()->bounded(40) / 100.0;
        r.color = colors[i % 6];
        results_.append(r);
    }

    updateInfo();
    emit taggingComplete(results_.size(), totalTagsGenerated());
    update();
}

QList<AutoTagResult> PaperAutoTagger::results() const { return results_; }

QMap<QString, int> PaperAutoTagger::tagFrequency() const {
    QMap<QString, int> freq;
    for (const auto& r : results_) {
        for (const auto& t : r.suggestedTags) freq[t]++;
    }
    return freq;
}

int PaperAutoTagger::totalTagsGenerated() const {
    int t = 0;
    for (const auto& r : results_) t += r.suggestedTags.size();
    return t;
}

void PaperAutoTagger::onAutoTag() { autoTag(); }
void PaperAutoTagger::onConfidenceChanged(int) { update(); }
void PaperAutoTagger::onClear() {
    results_.clear();
    infoLabel_->setText("Auto-tag papers with suggested labels");
    update();
}

void PaperAutoTagger::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (results_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Auto-tag papers with suggested labels");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Auto Tagger");

    int w = width(), h = height();
    drawTagCloud(p, QRect(20, 50, w - 40, h / 2 - 20));
    drawConfidenceChart(p, QRect(20, h / 2 + 20, w / 2 - 30, h / 2 - 50));
    drawPaperList(p, QRect(w / 2 + 10, h / 2 + 20, w / 2 - 30, h / 2 - 50));
}

void PaperAutoTagger::drawTagCloud(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Suggested Tags");

    auto freq = tagFrequency();
    QList<QPair<QString, int>> sorted;
    for (auto it = freq.begin(); it != freq.end(); ++it) sorted.append({it.key(), it.value()});
    std::sort(sorted.begin(), sorted.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    int maxFreq = sorted.isEmpty() ? 1 : sorted.first().second;
    int show = qMin(20, sorted.size());

    qreal x = rect.x() + 10;
    qreal y = rect.y() + 20;
    qreal rowH = 24;

    for (int i = 0; i < show; ++i) {
        qreal relSize = static_cast<qreal>(sorted[i].second) / maxFreq;
        int fontSize = qBound(8, static_cast<int>(9 + relSize * 8), 16);
        QFont::Weight weight = relSize > 0.5 ? QFont::Bold : QFont::Normal;

        QFontMetrics fm(QFont("Arial", fontSize));
        qreal tagW = fm.horizontalAdvance(sorted[i].first) + 16;

        if (x + tagW > rect.right() - 10) {
            x = rect.x() + 10;
            y += rowH;
        }

        QColor tagColor(59 + (i * 37) % 180, 130 + (i * 23) % 120, 246 - (i * 17) % 100);
        p.setPen(Qt::NoPen);
        p.setBrush(tagColor.lighter(170));
        p.drawRoundedRect(QRectF(x, y, tagW, rowH - 4).toRect(), 10, 10);

        p.setPen(tagColor);
        p.setFont(QFont("Arial", fontSize, weight));
        p.drawText(QRectF(x + 6, y + 1, tagW - 12, rowH - 6).toRect(), Qt::AlignVCenter, sorted[i].first);

        x += tagW + 6;
    }
}

void PaperAutoTagger::drawConfidenceChart(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Confidence Distribution");

    QMap<int, int> buckets;
    for (const auto& r : results_) {
        int bucket = static_cast<int>(r.confidence * 10);
        buckets[bucket]++;
    }

    int maxVal = 1;
    for (const auto& v : buckets) maxVal = qMax(maxVal, v);

    int barW = (rect.width() - 20) / 5;
    for (int i = 0; i < 5; ++i) {
        int x = rect.x() + 10 + i * barW;
        qreal val = buckets.contains(5 + i) ? buckets[5 + i] : 0;
        qreal h = (val / maxVal) * (rect.height() - 50);

        p.setPen(Qt::NoPen);
        p.setBrush(QColor(59, 130, 246));
        p.drawRoundedRect(x, rect.bottom() - 25 - static_cast<int>(h), barW - 4, static_cast<int>(h), 2, 2);

        p.setPen(QColor(100, 116, 139));
        p.setFont(QFont("Arial", 7));
        p.drawText(x, rect.bottom() - 8, barW, 12, Qt::AlignCenter,
                   QString("0.%1").arg(5 + i));
    }
}

void PaperAutoTagger::drawPaperList(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Papers");

    int show = qMin(8, results_.size());
    int itemH = qMin(30, (rect.height() - 25) / show);

    for (int i = 0; i < show; ++i) {
        const auto& r = results_[i];
        int y = rect.y() + 22 + i * itemH;

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y, rect.width() - 40, itemH, Qt::AlignVCenter,
                   r.paperTitle.left(20));

        p.setPen(r.color);
        p.setFont(QFont("Arial", 7));
        p.drawText(rect.x() + rect.width() - 40, y, 40, itemH, Qt::AlignVCenter | Qt::AlignRight,
                   QString::number(r.suggestedTags.size()) + " tags");
    }
}

void PaperAutoTagger::updateInfo() {
    if (results_.isEmpty()) { infoLabel_->setText("Auto-tag papers with suggested labels"); return; }
    infoLabel_->setText(QString("%1 papers | %2 tags | %3 unique tags")
        .arg(results_.size()).arg(totalTagsGenerated()).arg(tagFrequency().size()));
}
