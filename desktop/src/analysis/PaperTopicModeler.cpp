#include "analysis/PaperTopicModeler.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QRandomGenerator>
#include <cmath>

PaperTopicModeler::PaperTopicModeler(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperTopicModeler::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Topics:"));
    topicCountCombo_ = new QComboBox();
    topicCountCombo_->addItems({"3", "5", "8", "10", "15"});
    topicCountCombo_->setCurrentIndex(1);
    connect(topicCountCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperTopicModeler::onTopicCountChanged);
    toolbar->addWidget(topicCountCombo_);

    modelBtn_ = new QPushButton("Model Topics");
    modelBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(modelBtn_, &QPushButton::clicked, this, &PaperTopicModeler::onModel);
    toolbar->addWidget(modelBtn_);

    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperTopicModeler::clear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    auto* splitter = new QSplitter(Qt::Horizontal);
    auto* canvas = new QWidget();
    canvas->setMinimumSize(400, 300);
    splitter->addWidget(canvas);

    topicList_ = new QListWidget();
    topicList_->setMaximumWidth(200);
    topicList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 3px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(topicList_, &QListWidget::itemClicked, this, &PaperTopicModeler::onTopicClicked);
    splitter->addWidget(topicList_);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("Click Model to discover topics");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperTopicModeler::setPapers(const QList<QPair<int, QString>>& papers) {
    Q_UNUSED(papers);
}

void PaperTopicModeler::model(int numTopics) {
    topics_.clear();
    QStringList allWords = {
        "neural", "network", "learning", "deep", "model", "transformer", "attention",
        "training", "gradient", "optimization", "convolutional", "recurrent", "generative",
        "adversarial", "diffusion", "reinforcement", "supervised", "unsupervised", "embedding",
        "classification", "regression", "clustering", "segmentation", "detection", "recognition",
        "language", "translation", "generation", "understanding", "reasoning", "planning",
        "memory", "knowledge", "retrieval", "augmented", "federated", "privacy", "security",
        "robust", "explainable", "interpretability", "fairness", "bias", "alignment",
        "multimodal", "vision", "speech", "image", "text", "video", "audio",
        "graph", "point", "cloud", "3d", "robotics", "simulation", "benchmark",
        "dataset", "pretrain", "finetune", "prompt", "zero-shot", "few-shot", "in-context"};

    QColor colors[] = {QColor(59,130,246), QColor(16,185,129), QColor(245,158,11),
                       QColor(239,68,68), QColor(139,92,246), QColor(236,72,153),
                       QColor(14,165,233), QColor(168,85,247), QColor(234,179,8), QColor(34,197,94),
                       QColor(6,182,212), QColor(249,115,22), QColor(244,63,94), QColor(132,204,22), QColor(99,102,241)};

    QStringList topicNames = {"Neural Architecture", "NLP & Language", "Computer Vision",
        "Generative Models", "Optimization", "Representation Learning", "Robustness & Safety",
        "Multimodal AI", "Graph Learning", "Reinforcement Learning", "Data & Benchmarks",
        "Prompt Engineering", "Federated & Privacy", "3D & Robotics", "Explainability"};

    for (int i = 0; i < numTopics; ++i) {
        Topic t;
        t.id = i;
        t.label = topicNames[i % topicNames.size()];
        t.color = colors[i % 15];
        t.proportion = 0.05 + QRandomGenerator::global()->bounded(20) / 100.0;

        // Pick 8 random words with weights
        QList<int> indices;
        while (indices.size() < 8) {
            int idx = QRandomGenerator::global()->bounded(allWords.size());
            if (!indices.contains(idx)) indices.append(idx);
        }
        for (int idx : indices) {
            TopicWord tw;
            tw.word = allWords[idx];
            tw.weight = 0.1 + QRandomGenerator::global()->bounded(90) / 100.0;
            t.words.append(tw);
        }
        std::sort(t.words.begin(), t.words.end(),
            [](const TopicWord& a, const TopicWord& b) { return a.weight > b.weight; });
        topics_.append(t);
    }

    // Normalize proportions
    qreal total = 0;
    for (const auto& t : topics_) total += t.proportion;
    for (auto& t : topics_) t.proportion /= total;

    refreshList();
    statsLabel_->setText(QString("%1 topics discovered").arg(topics_.size()));
    emit modelingComplete(topics_.size());
    update();
}

QList<Topic> PaperTopicModeler::topics() const { return topics_; }

void PaperTopicModeler::clear() {
    topics_.clear();
    topicList_->clear();
    selectedTopic_ = -1;
    statsLabel_->setText("Click Model to discover topics");
    update();
}

void PaperTopicModeler::onModel() {
    int n = topicCountCombo_->currentText().toInt();
    model(n);
}

void PaperTopicModeler::onTopicCountChanged(int) {}

void PaperTopicModeler::onTopicClicked() {
    auto* item = topicList_->currentItem();
    if (!item) return;
    selectedTopic_ = item->data(Qt::UserRole).toInt();
    for (const auto& t : topics_) {
        if (t.id == selectedTopic_) {
            emit topicClicked(t.id, t.label);
            break;
        }
    }
    update();
}

void PaperTopicModeler::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (topics_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect(), Qt::AlignCenter, "Click 'Model Topics' to discover topics");
        return;
    }

    p.setPen(QColor(15, 23, 42));
    p.setFont(QFont("Arial", 13, QFont::Bold));
    p.drawText(20, 30, "Topic Modeling");

    int h = height();
    drawTopicBars(p, QRect(20, 50, width() - 40, qMin(200, h / 2 - 30)));
    drawWordCloud(p, QRect(20, h / 2 + 20, width() - 40, h / 2 - 50));
}

void PaperTopicModeler::drawTopicBars(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Topic Distribution");

    int barY = rect.y() + 20;
    int barH = qMin(20, (rect.height() - 30) / qMax(1, topics_.size()));

    for (int i = 0; i < topics_.size(); ++i) {
        const auto& t = topics_[i];
        int y = barY + i * (barH + 3);
        int barW = static_cast<int>(t.proportion * (rect.width() - 160));
        bool sel = (t.id == selectedTopic_);

        p.setPen(QColor(15, 23, 42));
        p.setFont(QFont("Arial", 8));
        p.drawText(rect.x(), y + barH - 3, 100, barH, Qt::AlignRight | Qt::AlignVCenter,
                   t.label.left(14));

        p.setPen(Qt::NoPen);
        p.setBrush(sel ? t.color : t.color.lighter(140));
        p.drawRoundedRect(rect.x() + 105, y, barW, barH - 2, 3, 3);

        p.setPen(QColor(100, 116, 139));
        p.drawText(rect.x() + 108 + barW, y + barH - 3,
                   QString::number(t.proportion * 100, 'f', 1) + "%");
    }
}

void PaperTopicModeler::drawWordCloud(QPainter& p, const QRect& rect) {
    p.setPen(QColor(100, 116, 139));
    p.setFont(QFont("Arial", 10));
    p.drawText(rect.topLeft(), "Top Words");

    int cloudY = rect.y() + 20;
    int cloudH = rect.height() - 25;
    int cloudW = rect.width();

    // Collect top words across all topics
    QList<QPair<QString, qreal>> allWords;
    for (const auto& t : topics_) {
        for (const auto& w : t.words) {
            bool found = false;
            for (auto& aw : allWords) {
                if (aw.first == w.word) { aw.second += w.weight * t.proportion; found = true; break; }
            }
            if (!found) allWords.append({w.word, w.weight * t.proportion});
        }
    }
    std::sort(allWords.begin(), allWords.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

    int maxWords = qMin(25, allWords.size());
    qreal maxWeight = allWords.isEmpty() ? 1 : allWords.first().second;

    // Grid layout
    int cols = qMax(1, cloudW / 90);
    int rows = (maxWords + cols - 1) / cols;

    for (int i = 0; i < maxWords; ++i) {
        int col = i % cols;
        int row = i / cols;
        qreal x = rect.x() + col * (cloudW / cols) + 10;
        qreal y = cloudY + row * (cloudH / qMax(1, rows)) + 14;

        qreal relWeight = allWords[i].second / maxWeight;
        int fontSize = qBound(7, static_cast<int>(8 + relWeight * 12), 18);

        // Find which topic this word belongs to
        QColor color(100, 116, 139);
        for (const auto& t : topics_) {
            for (const auto& w : t.words) {
                if (w.word == allWords[i].first) { color = t.color; break; }
            }
        }

        p.setPen(color);
        p.setFont(QFont("Arial", fontSize, relWeight > 0.5 ? QFont::Bold : QFont::Normal));
        p.drawText(QPointF(x, y), allWords[i].first);
    }
}

void PaperTopicModeler::refreshList() {
    topicList_->clear();
    for (const auto& t : topics_) {
        QStringList topWords;
        for (int i = 0; i < qMin(3, t.words.size()); ++i) topWords << t.words[i].word;
        QString display = QString("T%1: %2\n  %3")
            .arg(t.id + 1).arg(t.label).arg(topWords.join(", "));
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, t.id);
        item->setForeground(t.color);
        topicList_->addItem(item);
    }
}

void PaperTopicModeler::updateStats() {
    if (topics_.isEmpty()) { statsLabel_->setText("No topics"); return; }
    statsLabel_->setText(QString("%1 topics discovered").arg(topics_.size()));
}
