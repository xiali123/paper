#include "analysis/PaperDuplicateDetector.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>

PaperDuplicateDetector::PaperDuplicateDetector(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperDuplicateDetector::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "High (>90%)", "Medium (70-90%)", "Low (50-70%)"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperDuplicateDetector::onFilterChanged);
    toolbar->addWidget(filterCombo_, 1);

    detectBtn_ = new QPushButton("Detect");
    detectBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(detectBtn_, &QPushButton::clicked, this, &PaperDuplicateDetector::onDetect);
    toolbar->addWidget(detectBtn_);
    layout->addLayout(toolbar);

    progressBar_ = new QProgressBar();
    progressBar_->setRange(0, 100);
    progressBar_->setVisible(false);
    layout->addWidget(progressBar_);

    pairTree_ = new QTreeWidget();
    pairTree_->setHeaderLabels({"Paper 1", "Paper 2", "Similarity", "Type"});
    pairTree_->setColumnWidth(0, 220);
    pairTree_->setColumnWidth(1, 220);
    pairTree_->setColumnWidth(2, 70);
    pairTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(pairTree_, &QTreeWidget::itemClicked, this, &PaperDuplicateDetector::onPairSelected);
    layout->addWidget(pairTree_, 1);

    auto* btnRow = new QHBoxLayout();
    mergeBtn_ = new QPushButton("Merge Selected");
    mergeBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(mergeBtn_, &QPushButton::clicked, this, &PaperDuplicateDetector::onMerge);
    btnRow->addWidget(mergeBtn_);

    ignoreBtn_ = new QPushButton("Ignore");
    ignoreBtn_->setStyleSheet("color: #dc2626;");
    connect(ignoreBtn_, &QPushButton::clicked, this, &PaperDuplicateDetector::onIgnore);
    btnRow->addWidget(ignoreBtn_);
    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("Load papers and click Detect");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperDuplicateDetector::setPapers(const QList<QPair<int, QString>>& papers) {
    papers_ = papers;
    statsLabel_->setText(QString("%1 papers loaded").arg(papers_.size()));
}

void PaperDuplicateDetector::detect() {
    duplicates_.clear();
    progressBar_->setVisible(true);
    int total = papers_.size() * (papers_.size() - 1) / 2;
    int current = 0;

    for (int i = 0; i < papers_.size(); ++i) {
        for (int j = i + 1; j < papers_.size(); ++j) {
            qreal sim = computeSimilarity(papers_[i].second, papers_[j].second);
            if (sim >= 0.5) {
                DuplicatePair pair;
                pair.id1 = papers_[i].first;
                pair.id2 = papers_[j].first;
                pair.title1 = papers_[i].second;
                pair.title2 = papers_[j].second;
                pair.similarity = sim;
                if (sim >= 0.9) pair.matchType = "Exact";
                else if (sim >= 0.7) pair.matchType = "Similar";
                else pair.matchType = "Partial";
                duplicates_.append(pair);
            }
            current++;
            if (current % 100 == 0) progressBar_->setValue(current * 100 / qMax(1, total));
        }
    }

    progressBar_->setValue(100);
    int highCount = highConfidence().size();
    statsLabel_->setText(QString("%1 pairs found (%2 high confidence)").arg(duplicates_.size()).arg(highCount));
    refreshTree();
    emit detectionComplete(duplicates_.size(), highCount);
}

QList<DuplicatePair> PaperDuplicateDetector::duplicates() const { return duplicates_; }

QList<DuplicatePair> PaperDuplicateDetector::highConfidence() const {
    QList<DuplicatePair> result;
    for (const auto& p : duplicates_) {
        if (p.similarity >= 0.9) result.append(p);
    }
    return result;
}

void PaperDuplicateDetector::onDetect() { detect(); }

void PaperDuplicateDetector::onMerge() {
    if (selectedIdx_ < 0 || selectedIdx_ >= duplicates_.size()) return;
    const auto& pair = duplicates_[selectedIdx_];
    emit mergeRequested(pair.id1, pair.id2);
    duplicates_.removeAt(selectedIdx_);
    selectedIdx_ = -1;
    refreshTree();
    updateStats();
}

void PaperDuplicateDetector::onIgnore() {
    if (selectedIdx_ < 0 || selectedIdx_ >= duplicates_.size()) return;
    ignored_.append(duplicates_[selectedIdx_]);
    duplicates_.removeAt(selectedIdx_);
    selectedIdx_ = -1;
    refreshTree();
    updateStats();
}

void PaperDuplicateDetector::onFilterChanged(int) { refreshTree(); }

void PaperDuplicateDetector::onPairSelected() {
    auto* item = pairTree_->currentItem();
    if (!item) return;
    selectedIdx_ = item->data(0, Qt::UserRole).toInt();
}

qreal PaperDuplicateDetector::computeSimilarity(const QString& a, const QString& b) {
    if (a.isEmpty() || b.isEmpty()) return 0.0;
    QString al = a.toLower();
    QString bl = b.toLower();
    if (al == bl) return 1.0;

    // Bigram similarity
    QMap<QString, int> bigramsA, bigramsB;
    for (int i = 0; i < al.length() - 1; ++i) bigramsA[al.mid(i, 2)]++;
    for (int i = 0; i < bl.length() - 1; ++i) bigramsB[bl.mid(i, 2)]++;

    int intersection = 0;
    for (auto it = bigramsA.begin(); it != bigramsA.end(); ++it) {
        if (bigramsB.contains(it.key())) intersection += qMin(it.value(), bigramsB[it.key()]);
    }
    int totalA = 0, totalB = 0;
    for (const auto& v : bigramsA) totalA += v;
    for (const auto& v : bigramsB) totalB += v;

    qreal sim = static_cast<qreal>(2 * intersection) / qMax(1, totalA + totalB);

    // Word overlap bonus
    QStringList wordsA = al.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    QStringList wordsB = bl.split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
    int wordMatch = 0;
    for (const auto& w : wordsA) {
        if (wordsB.contains(w)) wordMatch++;
    }
    qreal wordSim = static_cast<qreal>(wordMatch) / qMax(1, qMax(wordsA.size(), wordsB.size()));

    return sim * 0.5 + wordSim * 0.5;
}

void PaperDuplicateDetector::refreshTree() {
    pairTree_->clear();
    int filter = filterCombo_->currentIndex();
    QMap<QString, QColor> typeColors = {
        {"Exact", QColor(239, 68, 68)}, {"Similar", QColor(245, 158, 11)}, {"Partial", QColor(59, 130, 246)}
    };

    for (int i = 0; i < duplicates_.size(); ++i) {
        const auto& p = duplicates_[i];
        if (filter == 1 && p.similarity < 0.9) continue;
        if (filter == 2 && (p.similarity < 0.7 || p.similarity >= 0.9)) continue;
        if (filter == 3 && (p.similarity < 0.5 || p.similarity >= 0.7)) continue;

        auto* item = new QTreeWidgetItem({
            p.title1.left(35),
            p.title2.left(35),
            QString::number(p.similarity * 100, 'f', 0) + "%",
            p.matchType
        });
        item->setData(0, Qt::UserRole, i);
        if (typeColors.contains(p.matchType)) item->setForeground(3, typeColors[p.matchType]);
        pairTree_->addTopLevelItem(item);
    }
}

void PaperDuplicateDetector::updateStats() {
    int high = highConfidence().size();
    statsLabel_->setText(QString("%1 duplicates (%2 high) | %3 ignored")
        .arg(duplicates_.size()).arg(high).arg(ignored_.size()));
}
