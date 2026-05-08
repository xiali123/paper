#include "paper/PaperGraderWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QDateTime>

double PaperGrade::weightedScore() const {
    double sum = 0;
    for (const auto& d : dimensions) sum += d.score * d.weight;
    return sum;
}

double PaperGrade::maxPossible() const {
    double sum = 0;
    for (const auto& d : dimensions) sum += d.maxScore * d.weight;
    return sum;
}

PaperGraderWidget::PaperGraderWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadDefaults();
    loadSettings();
}

void PaperGraderWidget::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: grade list
    auto* leftPanel = new QVBoxLayout();
    paperLabel_ = new QLabel("Select or grade a paper");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    leftPanel->addWidget(paperLabel_);

    gradeList_ = new QTableWidget();
    gradeList_->setColumnCount(3);
    gradeList_->setHorizontalHeaderLabels({"Paper", "Score", "Date"});
    gradeList_->horizontalHeader()->setStretchLastSection(true);
    gradeList_->setColumnWidth(0, 180);
    gradeList_->setColumnWidth(1, 80);
    gradeList_->setSelectionBehavior(QAbstractItemView::SelectRows);
    gradeList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    connect(gradeList_, &QTableWidget::cellClicked, this, &PaperGraderWidget::onPaperSelected);
    leftPanel->addWidget(gradeList_, 1);

    auto* btnRow = new QHBoxLayout();
    saveBtn_ = new QPushButton("Save Grade");
    saveBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 6px 16px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(saveBtn_, &QPushButton::clicked, this, &PaperGraderWidget::onSave);
    btnRow->addWidget(saveBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperGraderWidget::onDelete);
    btnRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export CSV");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperGraderWidget::onExport);
    btnRow->addWidget(exportBtn_);

    leftPanel->addLayout(btnRow);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    // Right: dimension sliders + comment
    auto* rightPanel = new QVBoxLayout();

    scoreLabel_ = new QLabel("Score: 0 / 0");
    scoreLabel_->setStyleSheet("font-size: 18px; font-weight: bold; color: #3b82f6;");
    scoreLabel_->setAlignment(Qt::AlignCenter);
    rightPanel->addWidget(scoreLabel_);

    dimensionTable_ = new QTableWidget();
    dimensionTable_->setColumnCount(4);
    dimensionTable_->setHorizontalHeaderLabels({"Dimension", "Weight", "Score", "Max"});
    dimensionTable_->horizontalHeader()->setStretchLastSection(true);
    dimensionTable_->setColumnWidth(0, 140);
    dimensionTable_->setColumnWidth(1, 60);
    dimensionTable_->setColumnWidth(2, 60);
    connect(dimensionTable_, &QTableWidget::cellChanged, this, &PaperGraderWidget::onDimensionChanged);
    rightPanel->addWidget(dimensionTable_, 1);

    rightPanel->addWidget(new QLabel("Overall Comment:"));
    commentEdit_ = new QTextEdit();
    commentEdit_->setMaximumHeight(100);
    rightPanel->addWidget(commentEdit_);

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);
}

void PaperGraderWidget::setPaper(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Paper: %1").arg(title));
    if (grades_.contains(paperId)) {
        loadGrade(paperId);
    } else {
        currentDims_[0].score = 0;
        refreshSliders();
        commentEdit_->clear();
        updateScoreDisplay();
    }
}

void PaperGraderWidget::setDimensions(const QList<GradeDimension>& dims) {
    currentDims_ = dims;
    refreshSliders();
}

void PaperGraderWidget::loadGrade(int paperId) {
    if (!grades_.contains(paperId)) return;
    const auto& grade = grades_[paperId];
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Paper: %1").arg(grade.paperTitle));
    currentDims_ = grade.dimensions;
    commentEdit_->setPlainText(grade.overallComment);
    refreshSliders();
    updateScoreDisplay();
}

QList<PaperGrade> PaperGraderWidget::allGrades() const { return grades_.values(); }

void PaperGraderWidget::exportGrades(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;
    QTextStream out(&file);
    out << "Paper ID,Title,Score,Max,Date,Comment\n";
    for (const auto& g : grades_) {
        out << QString("%1,\"%2\",%3,%4,%5,\"%6\"\n")
            .arg(g.paperId)
            .arg(g.paperTitle)
            .arg(g.weightedScore(), 0, 'f', 1)
            .arg(g.maxPossible(), 0, 'f', 0)
            .arg(QDateTime::fromSecsSinceEpoch(g.gradedAt).toString("yyyy-MM-dd"))
            .arg(g.overallComment);
    }
}

void PaperGraderWidget::onSave() {
    if (currentPaperId_ < 0) return;

    PaperGrade grade;
    grade.paperId = currentPaperId_;
    grade.paperTitle = paperLabel_->text().remove("Paper: ");
    grade.dimensions = currentDims_;
    grade.overallComment = commentEdit_->toPlainText();
    grade.gradedAt = QDateTime::currentSecsSinceEpoch();

    for (int i = 0; i < dimensionTable_->rowCount(); ++i) {
        if (i < grade.dimensions.size()) {
            auto* scoreItem = dimensionTable_->item(i, 2);
            if (scoreItem) grade.dimensions[i].score = scoreItem->text().toInt();
        }
    }

    grades_[currentPaperId_] = grade;
    saveSettings();
    refreshGradeList();
    updateScoreDisplay();
    emit gradeSaved(currentPaperId_, grade.weightedScore());
}

void PaperGraderWidget::onDelete() {
    if (currentPaperId_ < 0 || !grades_.contains(currentPaperId_)) return;
    grades_.remove(currentPaperId_);
    saveSettings();
    refreshGradeList();
    emit gradeDeleted(currentPaperId_);
}

void PaperGraderWidget::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Grades", "grades.csv", "CSV (*.csv)");
    if (path.isEmpty()) return;
    exportGrades(path);
    emit gradeExported(grades_.size());
}

void PaperGraderWidget::onDimensionChanged() {
    updateScoreDisplay();
}

void PaperGraderWidget::onPaperSelected(int row) {
    if (row < 0) return;
    int id = gradeList_->item(row, 0)->data(Qt::UserRole).toInt();
    loadGrade(id);
}

void PaperGraderWidget::loadDefaults() {
    currentDims_ = {
        {"Novelty", 3, 0, 10},
        {"Methodology", 2, 0, 10},
        {"Clarity", 2, 0, 10},
        {"Significance", 2, 0, 10},
        {"Reproducibility", 1, 0, 10},
    };
    refreshSliders();
}

void PaperGraderWidget::loadSettings() {
    QSettings settings("PaperCrawler", "PaperGrades");
    QByteArray data = settings.value("grades").toByteArray();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        PaperGrade g;
        g.paperId = obj["paperId"].toInt();
        g.paperTitle = obj["paperTitle"].toString();
        g.overallComment = obj["overallComment"].toString();
        g.gradedAt = obj["gradedAt"].toInteger();

        QJsonArray dims = obj["dimensions"].toArray();
        for (const auto& d : dims) {
            QJsonObject dobj = d.toObject();
            GradeDimension dim;
            dim.name = dobj["name"].toString();
            dim.weight = dobj["weight"].toInt();
            dim.score = dobj["score"].toInt();
            dim.maxScore = dobj["maxScore"].toInt();
            g.dimensions.append(dim);
        }
        grades_[g.paperId] = g;
    }
    refreshGradeList();
}

void PaperGraderWidget::saveSettings() {
    QJsonArray arr;
    for (const auto& g : grades_) {
        QJsonObject obj;
        obj["paperId"] = g.paperId;
        obj["paperTitle"] = g.paperTitle;
        obj["overallComment"] = g.overallComment;
        obj["gradedAt"] = static_cast<qint64>(g.gradedAt);

        QJsonArray dims;
        for (const auto& d : g.dimensions) {
            QJsonObject dobj;
            dobj["name"] = d.name;
            dobj["weight"] = d.weight;
            dobj["score"] = d.score;
            dobj["maxScore"] = d.maxScore;
            dims.append(dobj);
        }
        obj["dimensions"] = dims;
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "PaperGrades");
    settings.setValue("grades", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}

void PaperGraderWidget::refreshGradeList() {
    gradeList_->setRowCount(grades_.size());
    int row = 0;
    for (const auto& g : grades_) {
        auto* titleItem = new QTableWidgetItem(g.paperTitle);
        titleItem->setData(Qt::UserRole, g.paperId);
        gradeList_->setItem(row, 0, titleItem);

        double pct = (g.maxPossible() > 0) ? g.weightedScore() / g.maxPossible() * 100 : 0;
        auto* scoreItem = new QTableWidgetItem(QString("%1 (%2%)")
            .arg(g.weightedScore(), 0, 'f', 1).arg(pct, 0, 'f', 0));
        QColor color = (pct >= 80) ? QColor(5, 150, 105) :
                       (pct >= 60) ? QColor(245, 158, 11) : QColor(220, 38, 38);
        scoreItem->setForeground(color);
        gradeList_->setItem(row, 1, scoreItem);

        gradeList_->setItem(row, 2, new QTableWidgetItem(
            QDateTime::fromSecsSinceEpoch(g.gradedAt).toString("yyyy-MM-dd")));
        row++;
    }
}

void PaperGraderWidget::refreshSliders() {
    dimensionTable_->blockSignals(true);
    dimensionTable_->setRowCount(currentDims_.size());
    for (int i = 0; i < currentDims_.size(); ++i) {
        const auto& dim = currentDims_[i];
        dimensionTable_->setItem(i, 0, new QTableWidgetItem(dim.name));
        dimensionTable_->setItem(i, 1, new QTableWidgetItem(QString::number(dim.weight)));
        dimensionTable_->setItem(i, 2, new QTableWidgetItem(QString::number(dim.score)));
        dimensionTable_->setItem(i, 3, new QTableWidgetItem(QString::number(dim.maxScore)));
    }
    dimensionTable_->blockSignals(false);
}

void PaperGraderWidget::updateScoreDisplay() {
    double score = 0, maxScore = 0;
    for (int i = 0; i < currentDims_.size(); ++i) {
        int s = 0;
        auto* item = dimensionTable_->item(i, 2);
        if (item) s = item->text().toInt();
        score += s * currentDims_[i].weight;
        maxScore += currentDims_[i].maxScore * currentDims_[i].weight;
    }
    scoreLabel_->setText(QString("Score: %1 / %2").arg(score, 0, 'f', 0).arg(maxScore, 0, 'f', 0));
}
