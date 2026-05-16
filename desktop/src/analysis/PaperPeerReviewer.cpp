#include "analysis/PaperPeerReviewer.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PaperPeerReviewer::PaperPeerReviewer(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperPeerReviewer::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper to review");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Verdict:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Strong Accept", "Accept", "Weak Accept", "Borderline", "Weak Reject", "Reject"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperPeerReviewer::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    reviewTree_ = new QTreeWidget();
    reviewTree_->setHeaderLabels({"Reviewer", "Verdict", "Score", "Date"});
    reviewTree_->setColumnWidth(0, 100);
    reviewTree_->setColumnWidth(1, 100);
    reviewTree_->setColumnWidth(2, 60);
    reviewTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(reviewTree_, &QTreeWidget::itemClicked, this, &PaperPeerReviewer::onReviewSelected);
    splitter->addWidget(reviewTree_);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* metaRow = new QHBoxLayout();
    metaRow->addWidget(new QLabel("Reviewer:"));
    reviewerEdit_ = new QLineEdit();
    reviewerEdit_->setPlaceholderText("Reviewer name...");
    metaRow->addWidget(reviewerEdit_, 1);
    metaRow->addWidget(new QLabel("Verdict:"));
    verdictCombo_ = new QComboBox();
    verdictCombo_->addItems({"Strong Accept", "Accept", "Weak Accept", "Borderline", "Weak Reject", "Reject"});
    metaRow->addWidget(verdictCombo_);
    rightLayout->addLayout(metaRow);

    rightLayout->addWidget(new QLabel("Summary:"));
    summaryEdit_ = new QTextEdit();
    summaryEdit_->setMaximumHeight(50);
    summaryEdit_->setPlaceholderText("Brief summary of the paper...");
    rightLayout->addWidget(summaryEdit_);

    rightLayout->addWidget(new QLabel("Strengths:"));
    strengthsEdit_ = new QTextEdit();
    strengthsEdit_->setMaximumHeight(50);
    strengthsEdit_->setPlaceholderText("Key strengths...");
    rightLayout->addWidget(strengthsEdit_);

    rightLayout->addWidget(new QLabel("Weaknesses:"));
    weaknessesEdit_ = new QTextEdit();
    weaknessesEdit_->setMaximumHeight(50);
    weaknessesEdit_->setPlaceholderText("Key weaknesses...");
    rightLayout->addWidget(weaknessesEdit_);

    rightLayout->addWidget(new QLabel("Suggestions:"));
    suggestionsEdit_ = new QTextEdit();
    suggestionsEdit_->setMaximumHeight(50);
    suggestionsEdit_->setPlaceholderText("Suggestions for improvement...");
    rightLayout->addWidget(suggestionsEdit_);

    auto* btnRow = new QHBoxLayout();
    submitBtn_ = new QPushButton("Submit Review");
    submitBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(submitBtn_, &QPushButton::clicked, this, &PaperPeerReviewer::onSubmit);
    btnRow->addWidget(submitBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperPeerReviewer::onDelete);
    btnRow->addWidget(deleteBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 reviews");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperPeerReviewer::setPaper(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Review: %1").arg(title));
    refreshTree();
    updateStats();
}

void PaperPeerReviewer::addReview(const PeerReview& review) {
    PeerReview r = review;
    if (r.id < 0) r.id = nextId_++;
    if (r.timestamp == 0) r.timestamp = QDateTime::currentSecsSinceEpoch();
    if (r.paperId < 0) r.paperId = currentPaperId_;
    if (r.criteria.isEmpty()) {
        r.criteria = {{"Novelty", 0, 5}, {"Soundness", 0, 5}, {"Clarity", 0, 5},
                       {"Significance", 0, 5}, {"Relevance", 0, 5}};
    }
    reviews_.append(r);
    nextId_ = qMax(nextId_, r.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
    emit reviewAdded(r.paperId, r.id, r.verdict);
}

void PaperPeerReviewer::removeReview(int reviewId) {
    reviews_.removeIf([reviewId](const PeerReview& r) { return r.id == reviewId; });
    refreshTree();
    saveSettings();
    updateStats();
    emit reviewRemoved(reviewId);
}

QList<PeerReview> PaperPeerReviewer::reviews() const { return reviews_; }

QList<PeerReview> PaperPeerReviewer::reviewsForPaper(int paperId) const {
    QList<PeerReview> result;
    for (const auto& r : reviews_) {
        if (r.paperId == paperId) result.append(r);
    }
    return result;
}

qreal PaperPeerReviewer::averageScore(int reviewId) const {
    for (const auto& r : reviews_) {
        if (r.id == reviewId) {
            if (r.criteria.isEmpty()) return 0;
            qreal sum = 0;
            for (const auto& c : r.criteria) sum += c.score;
            return sum / r.criteria.size();
        }
    }
    return 0;
}

void PaperPeerReviewer::onSubmit() {
    if (currentPaperId_ < 0) return;
    PeerReview r;
    r.paperId = currentPaperId_;
    r.reviewer = reviewerEdit_->text().trimmed().isEmpty() ? "Anonymous" : reviewerEdit_->text().trimmed();
    r.verdict = verdictCombo_->currentText();
    r.summary = summaryEdit_->toPlainText();
    r.strengths = strengthsEdit_->toPlainText();
    r.weaknesses = weaknessesEdit_->toPlainText();
    r.suggestions = suggestionsEdit_->toPlainText();
    r.criteria = {{"Novelty", 3, 5}, {"Soundness", 3, 5}, {"Clarity", 4, 5},
                   {"Significance", 3, 5}, {"Relevance", 4, 5}};
    addReview(r);
    reviewerEdit_->clear();
    summaryEdit_->clear();
    strengthsEdit_->clear();
    weaknessesEdit_->clear();
    suggestionsEdit_->clear();
}

void PaperPeerReviewer::onDelete() {
    if (selectedId_ < 0) return;
    removeReview(selectedId_);
    selectedId_ = -1;
}

void PaperPeerReviewer::onReviewSelected() {
    auto* item = reviewTree_->currentItem();
    if (!item) return;
    selectedId_ = item->data(0, Qt::UserRole).toInt();
    for (const auto& r : reviews_) {
        if (r.id == selectedId_) {
            reviewerEdit_->setText(r.reviewer);
            int vi = verdictCombo_->findText(r.verdict);
            if (vi >= 0) verdictCombo_->setCurrentIndex(vi);
            summaryEdit_->setPlainText(r.summary);
            strengthsEdit_->setPlainText(r.strengths);
            weaknessesEdit_->setPlainText(r.weaknesses);
            suggestionsEdit_->setPlainText(r.suggestions);
            break;
        }
    }
}

void PaperPeerReviewer::onFilterChanged(int) { refreshTree(); }

void PaperPeerReviewer::refreshTree() {
    reviewTree_->clear();
    QString filter = filterCombo_->currentText();
    QMap<QString, QColor> verdictColors = {
        {"Strong Accept", QColor(5,150,105)}, {"Accept", QColor(16,185,129)},
        {"Weak Accept", QColor(59,130,246)}, {"Borderline", QColor(245,158,11)},
        {"Weak Reject", QColor(239,68,68)}, {"Reject", QColor(185,28,28)}
    };

    for (const auto& r : reviews_) {
        if (currentPaperId_ >= 0 && r.paperId != currentPaperId_) continue;
        if (filter != "All" && r.verdict != filter) continue;
        qreal avg = averageScore(r.id);
        auto* item = new QTreeWidgetItem({
            r.reviewer,
            r.verdict,
            QString::number(avg, 'f', 1),
            QDateTime::fromSecsSinceEpoch(r.timestamp).toString("MM-dd HH:mm")
        });
        item->setData(0, Qt::UserRole, r.id);
        if (verdictColors.contains(r.verdict)) item->setForeground(1, verdictColors[r.verdict]);
        reviewTree_->addTopLevelItem(item);
    }
}

void PaperPeerReviewer::updateStats() {
    int total = 0, accepts = 0;
    for (const auto& r : reviews_) {
        if (currentPaperId_ >= 0 && r.paperId != currentPaperId_) continue;
        total++;
        if (r.verdict.contains("Accept")) accepts++;
    }
    statsLabel_->setText(QString("%1 reviews (%2 positive)").arg(total).arg(accepts));
}

void PaperPeerReviewer::loadSettings() {
    QSettings settings("PaperCrawler", "PeerReviews");
    QByteArray data = settings.value("reviews").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        PeerReview r;
        r.id = obj["id"].toInt();
        r.paperId = obj["paperId"].toInt();
        r.paperTitle = obj["paperTitle"].toString();
        r.reviewer = obj["reviewer"].toString();
        r.verdict = obj["verdict"].toString();
        r.summary = obj["summary"].toString();
        r.strengths = obj["strengths"].toString();
        r.weaknesses = obj["weaknesses"].toString();
        r.suggestions = obj["suggestions"].toString();
        r.timestamp = obj["timestamp"].toInteger();
        reviews_.append(r);
        nextId_ = qMax(nextId_, r.id + 1);
    }
    refreshTree();
}

void PaperPeerReviewer::saveSettings() {
    QJsonArray arr;
    for (const auto& r : reviews_) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["paperId"] = r.paperId;
        obj["paperTitle"] = r.paperTitle;
        obj["reviewer"] = r.reviewer;
        obj["verdict"] = r.verdict;
        obj["summary"] = r.summary;
        obj["strengths"] = r.strengths;
        obj["weaknesses"] = r.weaknesses;
        obj["suggestions"] = r.suggestions;
        obj["timestamp"] = static_cast<qint64>(r.timestamp);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "PeerReviews");
    settings.setValue("reviews", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
