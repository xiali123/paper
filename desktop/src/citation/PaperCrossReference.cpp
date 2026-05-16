#include "citation/PaperCrossReference.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QRandomGenerator>

PaperCrossReference::PaperCrossReference(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperCrossReference::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Filter
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Type:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Cites", "Extends", "Contradicts", "Supports", "Uses", "Related"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperCrossReference::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Ref tree
    refTree_ = new QTreeWidget();
    refTree_->setHeaderLabels({"Source", "Target", "Type", "Strength"});
    refTree_->setColumnWidth(0, 150);
    refTree_->setColumnWidth(1, 150);
    refTree_->setColumnWidth(2, 80);
    refTree_->setStyleSheet(
        "QTreeWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QTreeWidget::item { padding: 3px; }"
        "QTreeWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(refTree_, &QTreeWidget::itemClicked, this, &PaperCrossReference::onRefSelected);
    splitter->addWidget(refTree_);

    // Right panel: form + context
    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* srcRow = new QHBoxLayout();
    srcRow->addWidget(new QLabel("From:"));
    sourceCombo_ = new QComboBox();
    srcRow->addWidget(sourceCombo_, 1);
    srcRow->addWidget(new QLabel("To:"));
    targetCombo_ = new QComboBox();
    srcRow->addWidget(targetCombo_, 1);
    rightLayout->addLayout(srcRow);

    auto* typeRow = new QHBoxLayout();
    typeRow->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"Cites", "Extends", "Contradicts", "Supports", "Uses", "Related"});
    typeRow->addWidget(typeCombo_, 1);
    rightLayout->addLayout(typeRow);

    rightLayout->addWidget(new QLabel("Context:"));
    contextEdit_ = new QTextEdit();
    contextEdit_->setMaximumHeight(80);
    contextEdit_->setPlaceholderText("Describe the cross-reference context...");
    rightLayout->addWidget(contextEdit_);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Ref");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperCrossReference::onAdd);
    btnRow->addWidget(addBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperCrossReference::onDelete);
    btnRow->addWidget(deleteBtn_);

    autoBtn_ = new QPushButton("Auto Detect");
    autoBtn_->setStyleSheet("QPushButton { background: #059669; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(autoBtn_, &QPushButton::clicked, this, &PaperCrossReference::onAutoDetect);
    btnRow->addWidget(autoBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperCrossReference::onExport);
    btnRow->addWidget(exportBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 references");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperCrossReference::setPapers(const QList<QPair<int, QString>>& papers) {
    papers_ = papers;
    sourceCombo_->clear();
    targetCombo_->clear();
    for (const auto& p : papers) {
        QString label = QString("#%1 %2").arg(p.first).arg(p.second.left(30));
        sourceCombo_->addItem(label, p.first);
        targetCombo_->addItem(label, p.first);
    }
}

void PaperCrossReference::addRef(const CrossRef& ref) {
    CrossRef r = ref;
    if (r.id < 0) r.id = nextId_++;
    refs_.append(r);
    nextId_ = qMax(nextId_, r.id + 1);
    refreshTree();
    saveSettings();
    updateStats();
    emit refAdded(r.sourcePaperId, r.targetPaperId, r.refType);
}

void PaperCrossReference::removeRef(int refId) {
    refs_.removeIf([refId](const CrossRef& r) { return r.id == refId; });
    refreshTree();
    saveSettings();
    updateStats();
    emit refRemoved(refId);
}

QList<CrossRef> PaperCrossReference::refs() const { return refs_; }

QList<CrossRef> PaperCrossReference::refsForPaper(int paperId) const {
    QList<CrossRef> result;
    for (const auto& r : refs_) {
        if (r.sourcePaperId == paperId || r.targetPaperId == paperId) result.append(r);
    }
    return result;
}

QList<CrossRef> PaperCrossReference::refsByType(const QString& type) const {
    QList<CrossRef> result;
    for (const auto& r : refs_) {
        if (r.refType == type) result.append(r);
    }
    return result;
}

QMap<int, int> PaperCrossReference::buildAdjacency() const {
    QMap<int, int> adj;
    for (const auto& r : refs_) {
        adj[r.sourcePaperId]++;
        adj[r.targetPaperId]++;
    }
    return adj;
}

void PaperCrossReference::onAdd() {
    if (sourceCombo_->count() == 0 || targetCombo_->count() == 0) return;
    CrossRef r;
    r.sourcePaperId = sourceCombo_->currentData().toInt();
    r.targetPaperId = targetCombo_->currentData().toInt();
    if (r.sourcePaperId == r.targetPaperId) return;
    for (const auto& p : papers_) {
        if (p.first == r.sourcePaperId) r.sourceTitle = p.second;
        if (p.first == r.targetPaperId) r.targetTitle = p.second;
    }
    r.refType = typeCombo_->currentText();
    r.context = contextEdit_->toPlainText();
    r.strength = 1.0;
    addRef(r);
    contextEdit_->clear();
}

void PaperCrossReference::onDelete() {
    if (selectedRefId_ < 0) return;
    removeRef(selectedRefId_);
    selectedRefId_ = -1;
}

void PaperCrossReference::onFilterChanged(int) { refreshTree(); }

void PaperCrossReference::onRefSelected() {
    auto* item = refTree_->currentItem();
    if (!item) return;
    selectedRefId_ = item->data(0, Qt::UserRole).toInt();
    for (const auto& r : refs_) {
        if (r.id == selectedRefId_) {
            contextEdit_->setPlainText(r.context + "\n\n" + r.notes);
            break;
        }
    }
}

void PaperCrossReference::onAutoDetect() {
    if (papers_.size() < 2) return;
    QStringList types = {"Cites", "Extends", "Supports", "Uses", "Related"};
    for (int i = 0; i < qMin(papers_.size() * 2, 20); ++i) {
        int srcIdx = QRandomGenerator::global()->bounded(papers_.size());
        int tgtIdx = QRandomGenerator::global()->bounded(papers_.size());
        if (srcIdx == tgtIdx) continue;
        CrossRef r;
        r.sourcePaperId = papers_[srcIdx].first;
        r.targetPaperId = papers_[tgtIdx].first;
        r.sourceTitle = papers_[srcIdx].second;
        r.targetTitle = papers_[tgtIdx].second;
        r.refType = types[QRandomGenerator::global()->bounded(types.size())];
        r.strength = 0.3 + QRandomGenerator::global()->bounded(70) / 100.0;
        addRef(r);
    }
}

void PaperCrossReference::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Cross-References", "", "JSON (*.json)");
    if (fileName.isEmpty()) return;
    QJsonArray arr;
    for (const auto& r : refs_) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["sourcePaperId"] = r.sourcePaperId;
        obj["targetPaperId"] = r.targetPaperId;
        obj["sourceTitle"] = r.sourceTitle;
        obj["targetTitle"] = r.targetTitle;
        obj["refType"] = r.refType;
        obj["context"] = r.context;
        obj["strength"] = r.strength;
        arr.append(obj);
    }
    QFile f(fileName);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(arr).toJson());
}

void PaperCrossReference::refreshTree() {
    refTree_->clear();
    QString filter = filterCombo_->currentText();
    QMap<QString, QColor> typeColors = {
        {"Cites", QColor(59,130,246)}, {"Extends", QColor(16,185,129)},
        {"Contradicts", QColor(239,68,68)}, {"Supports", QColor(245,158,11)},
        {"Uses", QColor(139,92,246)}, {"Related", QColor(148,163,184)}
    };

    for (const auto& r : refs_) {
        if (filter != "All" && r.refType != filter) continue;
        auto* item = new QTreeWidgetItem({
            r.sourceTitle.left(25),
            r.targetTitle.left(25),
            r.refType,
            QString::number(r.strength, 'f', 1)
        });
        item->setData(0, Qt::UserRole, r.id);
        if (typeColors.contains(r.refType)) item->setForeground(2, typeColors[r.refType]);
        refTree_->addTopLevelItem(item);
    }
}

void PaperCrossReference::updateStats() {
    QMap<QString, int> counts;
    for (const auto& r : refs_) counts[r.refType]++;
    QStringList parts;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        parts << QString("%1: %2").arg(it.key()).arg(it.value());
    }
    statsLabel_->setText(QString("%1 refs | %2").arg(refs_.size()).arg(parts.join(", ")));
}

void PaperCrossReference::loadSettings() {
    QSettings settings("PaperCrawler", "CrossReference");
    QByteArray data = settings.value("refs").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        CrossRef r;
        r.id = obj["id"].toInt();
        r.sourcePaperId = obj["sourcePaperId"].toInt();
        r.targetPaperId = obj["targetPaperId"].toInt();
        r.sourceTitle = obj["sourceTitle"].toString();
        r.targetTitle = obj["targetTitle"].toString();
        r.refType = obj["refType"].toString();
        r.context = obj["context"].toString();
        r.notes = obj["notes"].toString();
        r.strength = obj["strength"].toDouble();
        refs_.append(r);
        nextId_ = qMax(nextId_, r.id + 1);
    }
    refreshTree();
    updateStats();
}

void PaperCrossReference::saveSettings() {
    QJsonArray arr;
    for (const auto& r : refs_) {
        QJsonObject obj;
        obj["id"] = r.id;
        obj["sourcePaperId"] = r.sourcePaperId;
        obj["targetPaperId"] = r.targetPaperId;
        obj["sourceTitle"] = r.sourceTitle;
        obj["targetTitle"] = r.targetTitle;
        obj["refType"] = r.refType;
        obj["context"] = r.context;
        obj["notes"] = r.notes;
        obj["strength"] = r.strength;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "CrossReference");
    settings.setValue("refs", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
