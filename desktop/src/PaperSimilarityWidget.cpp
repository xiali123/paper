#include "PaperSimilarityWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <algorithm>

PaperSimilarityWidget::PaperSimilarityWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperSimilarityWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* headerRow = new QHBoxLayout();
    headerRow->addWidget(new QLabel("Method:"));
    methodCombo_ = new QComboBox();
    methodCombo_->addItems({"Jaccard (Keywords)", "Cosine Similarity", "Title Overlap"});
    connect(methodCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperSimilarityWidget::onMethodChanged);
    headerRow->addWidget(methodCombo_, 1);

    computeBtn_ = new QPushButton("Compute");
    computeBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(computeBtn_, &QPushButton::clicked, this, &PaperSimilarityWidget::onCompute);
    headerRow->addWidget(computeBtn_);

    auto* exportBtn = new QPushButton("Export");
    connect(exportBtn, &QPushButton::clicked, this, &PaperSimilarityWidget::onExport);
    headerRow->addWidget(exportBtn);

    layout->addLayout(headerRow);

    statsLabel_ = new QLabel("Load papers and compute similarities");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);

    table_ = new QTableWidget();
    table_->setColumnCount(4);
    table_->setHorizontalHeaderLabels({"Paper A", "Paper B", "Score", "Method"});
    table_->horizontalHeader()->setStretchLastSection(true);
    table_->setColumnWidth(0, 200);
    table_->setColumnWidth(1, 200);
    table_->setColumnWidth(2, 60);
    table_->setSelectionBehavior(QAbstractItemView::SelectRows);
    table_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(table_, &QTableWidget::cellClicked, this, &PaperSimilarityWidget::onCellClicked);
    layout->addWidget(table_, 1);
}

void PaperSimilarityWidget::setPapers(const QList<QPair<int, QString>>& papers) {
    papers_ = papers;
    statsLabel_->setText(QString("%1 papers loaded").arg(papers_.size()));
}

void PaperSimilarityWidget::setMethod(const QString& method) {
    method_ = method;
}

QList<SimilarityPair> PaperSimilarityWidget::computeSimilarities() {
    results_.clear();
    int n = papers_.size();
    if (n < 2) return results_;

    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            double score = (method_ == "cosine")
                ? cosineSim(papers_[i].second, papers_[j].second)
                : jaccardSim(papers_[i].second, papers_[j].second);

            if (score >= threshold_) {
                SimilarityPair pair;
                pair.paperA = papers_[i].first;
                pair.paperB = papers_[j].first;
                pair.score = score;
                pair.method = method_;
                results_.append(pair);
            }
        }
    }

    std::sort(results_.begin(), results_.end(),
        [](const SimilarityPair& a, const SimilarityPair& b) { return a.score > b.score; });

    return results_;
}

void PaperSimilarityWidget::onCompute() {
    results_ = computeSimilarities();

    table_->setRowCount(results_.size());
    for (int i = 0; i < results_.size(); ++i) {
        const auto& r = results_[i];
        table_->setItem(i, 0, new QTableWidgetItem("Paper #" + QString::number(r.paperA)));
        table_->setItem(i, 1, new QTableWidgetItem("Paper #" + QString::number(r.paperB)));

        auto* scoreItem = new QTableWidgetItem(QString::number(r.score, 'f', 3));
        QColor color = (r.score >= 0.8) ? QColor(34, 197, 94) :
                       (r.score >= 0.5) ? QColor(245, 158, 11) : QColor(100, 116, 139);
        scoreItem->setForeground(color);
        table_->setItem(i, 2, scoreItem);

        table_->setItem(i, 3, new QTableWidgetItem(r.method));
    }

    statsLabel_->setText(QString("%1 similar pairs found (threshold: %2)")
        .arg(results_.size()).arg(threshold_, 0, 'f', 1));
    emit computationDone(results_.size());
}

void PaperSimilarityWidget::onMethodChanged(int index) {
    switch (index) {
        case 0: method_ = "jaccard"; break;
        case 1: method_ = "cosine"; break;
        case 2: method_ = "title"; break;
    }
}

void PaperSimilarityWidget::onCellClicked(int row, int) {
    if (row < 0 || row >= results_.size()) return;
    const auto& r = results_[row];
    emit pairClicked(r.paperA, r.paperB);
}

void PaperSimilarityWidget::onExport() {
    // Export results to clipboard
    QStringList lines;
    lines << "Paper A\tPaper B\tScore\tMethod";
    for (const auto& r : results_) {
        lines << QString("%1\t%2\t%3\t%4")
            .arg(r.paperA).arg(r.paperB)
            .arg(r.score, 0, 'f', 3).arg(r.method);
    }
    QApplication::clipboard()->setText(lines.join("\n"));
    statsLabel_->setText("Copied to clipboard");
}

double PaperSimilarityWidget::jaccardSim(const QString& a, const QString& b) const {
    auto split = [](const QString& s) {
        return QSet<QString>(s.toLower().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts).begin(),
                             s.toLower().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts).end());
    };
    QSet<QString> sa = split(a), sb = split(b);
    if (sa.isEmpty() && sb.isEmpty()) return 1.0;
    int inter = 0;
    for (const auto& w : sa) if (sb.contains(w)) inter++;
    int uni = sa.size() + sb.size() - inter;
    return (uni == 0) ? 0.0 : static_cast<double>(inter) / uni;
}

double PaperSimilarityWidget::cosineSim(const QString& a, const QString& b) const {
    QMap<QString, int> freqA, freqB;
    for (const auto& w : a.toLower().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts))
        freqA[w]++;
    for (const auto& w : b.toLower().split(QRegularExpression("[,;\\s]+"), Qt::SkipEmptyParts))
        freqB[w]++;

    double dot = 0, magA = 0, magB = 0;
    for (auto it = freqA.constBegin(); it != freqA.constEnd(); ++it) {
        if (freqB.contains(it.key())) dot += it.value() * freqB[it.key()];
        magA += it.value() * it.value();
    }
    for (auto it = freqB.constBegin(); it != freqB.constEnd(); ++it)
        magB += it.value() * it.value();

    double denom = sqrt(magA) * sqrt(magB);
    return (denom == 0) ? 0.0 : dot / denom;
}
