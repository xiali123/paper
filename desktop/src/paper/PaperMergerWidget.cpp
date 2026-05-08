#include "paper/PaperMergerWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QApplication>

QStringList PaperMergerWidget::fieldNames() {
    return {"title", "authors", "year", "journal", "doi", "abstractText", "keywords", "pdfUrl", "rating"};
}

PaperMergerWidget::PaperMergerWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperMergerWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    infoLabel_ = new QLabel("Select which value to keep for each field");
    infoLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(infoLabel_);

    mergeTable_ = new QTableWidget();
    mergeTable_->setColumnCount(4);
    mergeTable_->setHorizontalHeaderLabels({"Field", "Paper A", "Paper B", "Keep"});
    mergeTable_->horizontalHeader()->setStretchLastSection(true);
    mergeTable_->setColumnWidth(0, 100);
    mergeTable_->setColumnWidth(1, 250);
    mergeTable_->setColumnWidth(2, 250);
    mergeTable_->setColumnWidth(3, 60);
    mergeTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    mergeTable_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    layout->addWidget(mergeTable_, 1);

    previewLabel_ = new QLabel("Merged result preview will appear here");
    previewLabel_->setWordWrap(true);
    previewLabel_->setStyleSheet("padding: 8px; background: #f0fdf4; border: 1px solid #bbf7d0; border-radius: 6px; font-size: 12px;");
    previewLabel_->setMaximumHeight(80);
    layout->addWidget(previewLabel_);

    auto* btnRow = new QHBoxLayout();
    autoABtn_ = new QPushButton("Keep All A");
    autoABtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(autoABtn_, &QPushButton::clicked, this, &PaperMergerWidget::onAutoSelectA);
    btnRow->addWidget(autoABtn_);

    autoBBtn_ = new QPushButton("Keep All B");
    autoBBtn_->setStyleSheet("QPushButton { background: #8b5cf6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(autoBBtn_, &QPushButton::clicked, this, &PaperMergerWidget::onAutoSelectB);
    btnRow->addWidget(autoBBtn_);

    btnRow->addStretch();

    cancelBtn_ = new QPushButton("Cancel");
    connect(cancelBtn_, &QPushButton::clicked, this, &PaperMergerWidget::onCancel);
    btnRow->addWidget(cancelBtn_);

    mergeBtn_ = new QPushButton("Merge");
    mergeBtn_->setStyleSheet(
        "QPushButton { background: #059669; color: white; padding: 6px 20px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(mergeBtn_, &QPushButton::clicked, this, &PaperMergerWidget::onMerge);
    btnRow->addWidget(mergeBtn_);

    layout->addLayout(btnRow);
}

void PaperMergerWidget::setPapers(const MergePaper& paperA, const MergePaper& paperB) {
    paperA_ = paperA;
    paperB_ = paperB;
    for (const auto& field : fieldNames()) fieldChoices_[field] = 0;
    refreshTable();
    updatePreview();
}

void PaperMergerWidget::autoMerge(int keepId) {
    int choice = (keepId == paperA_.id) ? 0 : 1;
    for (const auto& field : fieldNames()) fieldChoices_[field] = choice;
    refreshTable();
    updatePreview();
}

MergePaper PaperMergerWidget::mergedResult() const { return result_; }

void PaperMergerWidget::onMerge() {
    result_.id = (fieldChoices_["title"] == 0) ? paperA_.id : paperB_.id;
    int removeId = (result_.id == paperA_.id) ? paperB_.id : paperA_.id;

    auto getA = [&](const QString& f) -> QString {
        if (f == "title") return paperA_.title;
        if (f == "authors") return paperA_.authors;
        if (f == "year") return QString::number(paperA_.year);
        if (f == "journal") return paperA_.journal;
        if (f == "doi") return paperA_.doi;
        if (f == "abstractText") return paperA_.abstractText;
        if (f == "keywords") return paperA_.keywords;
        if (f == "pdfUrl") return paperA_.pdfUrl;
        if (f == "rating") return QString::number(paperA_.rating);
        return "";
    };
    auto getB = [&](const QString& f) -> QString {
        if (f == "title") return paperB_.title;
        if (f == "authors") return paperB_.authors;
        if (f == "year") return QString::number(paperB_.year);
        if (f == "journal") return paperB_.journal;
        if (f == "doi") return paperB_.doi;
        if (f == "abstractText") return paperB_.abstractText;
        if (f == "keywords") return paperB_.keywords;
        if (f == "pdfUrl") return paperB_.pdfUrl;
        if (f == "rating") return QString::number(paperB_.rating);
        return "";
    };

    auto pick = [&](const QString& f) -> QString {
        return (fieldChoices_[f] == 0) ? getA(f) : getB(f);
    };

    result_.title = pick("title");
    result_.authors = pick("authors");
    result_.year = pick("year").toInt();
    result_.journal = pick("journal");
    result_.doi = pick("doi");
    result_.abstractText = pick("abstractText");
    result_.keywords = pick("keywords");
    result_.pdfUrl = pick("pdfUrl");
    result_.rating = pick("rating").toInt();

    emit mergeCompleted(result_.id, removeId);
}

void PaperMergerWidget::onCancel() { emit mergeCancelled(); }
void PaperMergerWidget::onAutoSelectA() { autoMerge(paperA_.id); }
void PaperMergerWidget::onAutoSelectB() { autoMerge(paperB_.id); }

void PaperMergerWidget::refreshTable() {
    auto getA = [&](const QString& f) -> QString {
        if (f == "title") return paperA_.title;
        if (f == "authors") return paperA_.authors;
        if (f == "year") return QString::number(paperA_.year);
        if (f == "journal") return paperA_.journal;
        if (f == "doi") return paperA_.doi;
        if (f == "abstractText") return paperA_.abstractText.left(100);
        if (f == "keywords") return paperA_.keywords;
        if (f == "pdfUrl") return paperA_.pdfUrl;
        if (f == "rating") return QString::number(paperA_.rating);
        return "";
    };
    auto getB = [&](const QString& f) -> QString {
        if (f == "title") return paperB_.title;
        if (f == "authors") return paperB_.authors;
        if (f == "year") return QString::number(paperB_.year);
        if (f == "journal") return paperB_.journal;
        if (f == "doi") return paperB_.doi;
        if (f == "abstractText") return paperB_.abstractText.left(100);
        if (f == "keywords") return paperB_.keywords;
        if (f == "pdfUrl") return paperB_.pdfUrl;
        if (f == "rating") return QString::number(paperB_.rating);
        return "";
    };

    QStringList fields = fieldNames();
    QStringList labels = {"Title", "Authors", "Year", "Journal", "DOI", "Abstract", "Keywords", "PDF URL", "Rating"};
    mergeTable_->setRowCount(fields.size());

    for (int i = 0; i < fields.size(); ++i) {
        const QString& f = fields[i];
        mergeTable_->setItem(i, 0, new QTableWidgetItem(labels[i]));

        auto* aItem = new QTableWidgetItem(getA(f));
        aItem->setData(Qt::UserRole, "A");
        mergeTable_->setItem(i, 1, aItem);

        auto* bItem = new QTableWidgetItem(getB(f));
        bItem->setData(Qt::UserRole, "B");
        mergeTable_->setItem(i, 2, bItem);

        auto* combo = new QComboBox();
        combo->addItems({"A", "B"});
        combo->setCurrentIndex(fieldChoices_[f]);
        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, f, combo]() {
            fieldChoices_[f] = combo->currentIndex();
            updatePreview();
        });
        mergeTable_->setCellWidget(i, 3, combo);

        // Highlight differences
        if (getA(f) != getB(f)) {
            aItem->setBackground(QColor(254, 243, 199));
            bItem->setBackground(QColor(254, 243, 199));
        }
    }
}

void PaperMergerWidget::updatePreview() {
    QStringList preview;
    auto getVal = [&](const QString& f, int choice) -> QString {
        return (choice == 0) ?
            (f == "title" ? paperA_.title : f == "authors" ? paperA_.authors : "") :
            (f == "title" ? paperB_.title : f == "authors" ? paperB_.authors : "");
    };

    preview << QString("Title: %1").arg(
        fieldChoices_["title"] == 0 ? paperA_.title : paperB_.title);
    preview << QString("Authors: %1").arg(
        fieldChoices_["authors"] == 0 ? paperA_.authors : paperB_.authors);
    preview << QString("Year: %1 | Journal: %2")
        .arg(fieldChoices_["year"] == 0 ? paperA_.year : paperB_.year)
        .arg(fieldChoices_["journal"] == 0 ? paperA_.journal : paperB_.journal);

    previewLabel_->setText(preview.join("\n"));
}
