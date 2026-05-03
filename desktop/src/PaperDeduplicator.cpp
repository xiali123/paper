#include "PaperDeduplicator.hpp"
#include "PaperTypes.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QRegularExpression>
#include <algorithm>

PaperDeduplicator::PaperDeduplicator(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperDeduplicator::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Header
    auto* headerRow = new QHBoxLayout();
    statusLabel_ = new QLabel("Load papers and scan for duplicates");
    statusLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    headerRow->addWidget(statusLabel_);
    headerRow->addStretch();

    progressLabel_ = new QLabel("");
    progressLabel_->setStyleSheet("color: palette(mid); font-size: 11px;");
    headerRow->addWidget(progressLabel_);
    layout->addLayout(headerRow);

    // Scan button
    auto* actionRow = new QHBoxLayout();
    scanBtn_ = new QPushButton("Scan for Duplicates");
    scanBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 16px; "
        "border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(scanBtn_, &QPushButton::clicked, this, &PaperDeduplicator::onScan);
    actionRow->addWidget(scanBtn_);
    actionRow->addStretch();

    mergeBtn_ = new QPushButton("Merge (Keep Left)");
    mergeBtn_->setEnabled(false);
    connect(mergeBtn_, &QPushButton::clicked, this, &PaperDeduplicator::onMergeSelected);
    actionRow->addWidget(mergeBtn_);

    skipBtn_ = new QPushButton("Skip");
    skipBtn_->setEnabled(false);
    connect(skipBtn_, &QPushButton::clicked, this, &PaperDeduplicator::onSkip);
    actionRow->addWidget(skipBtn_);

    mergeAllBtn_ = new QPushButton("Merge All");
    mergeAllBtn_->setEnabled(false);
    connect(mergeAllBtn_, &QPushButton::clicked, this, &PaperDeduplicator::onMergeAll);
    actionRow->addWidget(mergeAllBtn_);

    layout->addLayout(actionRow);

    // Side by side tables
    auto* tablesRow = new QHBoxLayout();

    auto* leftPanel = new QWidget();
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    auto* leftLabel = new QLabel("Paper A (Keep)");
    leftLabel->setStyleSheet("font-weight: bold; color: #059669;");
    leftLayout->addWidget(leftLabel);

    leftTable_ = new QTableWidget();
    leftTable_->setColumnCount(2);
    leftTable_->setHorizontalHeaderLabels({"Field", "Value"});
    leftTable_->horizontalHeader()->setStretchLastSection(true);
    leftTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    leftTable_->setAlternatingRowColors(true);
    leftLayout->addWidget(leftTable_);
    tablesRow->addWidget(leftPanel, 1);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    auto* rightLabel = new QLabel("Paper B (Remove)");
    rightLabel->setStyleSheet("font-weight: bold; color: #dc2626;");
    rightLayout->addWidget(rightLabel);

    rightTable_ = new QTableWidget();
    rightTable_->setColumnCount(2);
    rightTable_->setHorizontalHeaderLabels({"Field", "Value"});
    rightTable_->horizontalHeader()->setStretchLastSection(true);
    rightTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    rightTable_->setAlternatingRowColors(true);
    rightLayout->addWidget(rightTable_);
    tablesRow->addWidget(rightPanel, 1);

    layout->addLayout(tablesRow, 1);
}

void PaperDeduplicator::setPapers(const QList<Paper>& papers) {
    papers_ = papers;
    statusLabel_->setText(QString("%1 papers loaded").arg(papers.size()));
}

QList<QPair<int, int>> PaperDeduplicator::findDuplicates() const {
    QList<QPair<int, int>> pairs;

    for (int i = 0; i < papers_.size(); ++i) {
        for (int j = i + 1; j < papers_.size(); ++j) {
            const auto& a = papers_[i];
            const auto& b = papers_[j];

            bool isDuplicate = false;

            // Same DOI
            if (!a.doiUrl.isEmpty() && a.doiUrl == b.doiUrl) {
                isDuplicate = true;
            }

            // Very similar title (simple check)
            QString ta = a.title.toLower().trimmed();
            QString tb = b.title.toLower().trimmed();
            if (ta.length() > 10 && tb.length() > 10) {
                // Check if one contains the other or if edit distance is small
                if (ta == tb) {
                    isDuplicate = true;
                } else if (ta.contains(tb) || tb.contains(ta)) {
                    isDuplicate = true;
                } else {
                    // Simple word overlap ratio
                    QStringList wa = ta.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    QStringList wb = tb.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
                    int common = 0;
                    for (const auto& w : wa) {
                        if (wb.contains(w)) common++;
                    }
                    float ratio = (2.0f * common) / (wa.size() + wb.size());
                    if (ratio > 0.85f) isDuplicate = true;
                }
            }

            // Same authors + same year
            if (!isDuplicate && a.year == b.year && !a.year.isEmpty()) {
                QString aa = a.authors.toLower().trimmed();
                QString ab = b.authors.toLower().trimmed();
                if (aa == ab && !aa.isEmpty()) {
                    // Same authors, same year — likely duplicate
                    isDuplicate = true;
                }
            }

            if (isDuplicate) {
                pairs.append({a.id, b.id});
            }
        }
    }

    return pairs;
}

void PaperDeduplicator::onScan() {
    duplicatePairs_ = findDuplicates();
    currentPair_ = 0;
    statusLabel_->setText(QString("Found %1 duplicate pairs").arg(duplicatePairs_.size()));

    if (duplicatePairs_.isEmpty()) {
        progressLabel_->setText("No duplicates found");
        mergeBtn_->setEnabled(false);
        skipBtn_->setEnabled(false);
        mergeAllBtn_->setEnabled(false);
        return;
    }

    mergeBtn_->setEnabled(true);
    skipBtn_->setEnabled(true);
    mergeAllBtn_->setEnabled(true);
    showNextPair();
}

void PaperDeduplicator::showNextPair() {
    if (currentPair_ >= duplicatePairs_.size()) {
        statusLabel_->setText("All pairs reviewed");
        progressLabel_->setText("");
        mergeBtn_->setEnabled(false);
        skipBtn_->setEnabled(false);
        return;
    }

    progressLabel_->setText(QString("Pair %1 / %2").arg(currentPair_ + 1).arg(duplicatePairs_.size()));

    int idA = duplicatePairs_[currentPair_].first;
    int idB = duplicatePairs_[currentPair_].second;

    const Paper* pA = nullptr;
    const Paper* pB = nullptr;
    for (const auto& p : papers_) {
        if (p.id == idA) pA = &p;
        if (p.id == idB) pB = &p;
    }

    auto fillTable = [](QTableWidget* table, const Paper* p) {
        table->setRowCount(0);
        if (!p) return;
        auto addRow = [&](const QString& field, const QString& val) {
            int r = table->rowCount();
            table->insertRow(r);
            table->setItem(r, 0, new QTableWidgetItem(field));
            table->setItem(r, 1, new QTableWidgetItem(val));
        };
        addRow("ID", QString::number(p->id));
        addRow("Title", p->title);
        addRow("Authors", p->authors);
        addRow("Year", p->year);
        addRow("Journal", p->journalFull.isEmpty() ? p->journal : p->journalFull);
        addRow("DOI", p->doiUrl);
        addRow("Source", p->source);
        addRow("Citations", QString::number(p->citationCount));
        addRow("Abstract", p->abstract.left(200));
        table->resizeColumnsToContents();
    };

    fillTable(leftTable_, pA);
    fillTable(rightTable_, pB);
}

void PaperDeduplicator::onMergeSelected() {
    if (currentPair_ >= duplicatePairs_.size()) return;
    int keepId = duplicatePairs_[currentPair_].first;
    int removeId = duplicatePairs_[currentPair_].second;
    emit mergeRequested(keepId, removeId);
    currentPair_++;
    showNextPair();
}

void PaperDeduplicator::onSkip() {
    currentPair_++;
    showNextPair();
}

void PaperDeduplicator::onMergeAll() {
    auto result = QMessageBox::question(this, "Merge All",
        QString("Merge all %1 duplicate pairs? The first paper in each pair will be kept.")
            .arg(duplicatePairs_.size() - currentPair_));
    if (result != QMessageBox::Yes) return;

    int count = 0;
    while (currentPair_ < duplicatePairs_.size()) {
        emit mergeRequested(duplicatePairs_[currentPair_].first,
                            duplicatePairs_[currentPair_].second);
        currentPair_++;
        count++;
    }
    emit papersMerged(count);
    statusLabel_->setText(QString("Merged %1 pairs").arg(count));
    mergeBtn_->setEnabled(false);
    skipBtn_->setEnabled(false);
    mergeAllBtn_->setEnabled(false);
}
