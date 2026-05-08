#include "LiteratureMatrixWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileDialog>
#include <QRandomGenerator>

LiteratureMatrixWidget::LiteratureMatrixWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "LiteratureMatrix")
{
    setupUI();
    loadSettings();
}

void LiteratureMatrixWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    toolbar->addWidget(new QLabel("Mode:"));
    modeCombo_ = new QComboBox();
    modeCombo_->addItems({"Comparison", "Coverage", "Scoring", "Heatmap"});
    connect(modeCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LiteratureMatrixWidget::onModeChanged);
    toolbar->addWidget(modeCombo_, 1);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &LiteratureMatrixWidget::onExport);
    toolbar->addWidget(exportBtn_);

    resetBtn_ = new QPushButton("Reset");
    resetBtn_->setStyleSheet("color: #dc2626;");
    connect(resetBtn_, &QPushButton::clicked, this, &LiteratureMatrixWidget::onReset);
    toolbar->addWidget(resetBtn_);
    layout->addLayout(toolbar);

    matrixTable_ = new QTableWidget();
    matrixTable_->setStyleSheet(
        "QTableWidget { border: 1px solid palette(mid); border-radius: 4px; gridline-color: #e2e8f0; }"
        "QTableWidget::item { padding: 4px; }"
        "QTableWidget::item:selected { background: #3b82f6; color: white; }"
        "QHeaderView::section { background: #f1f5f9; padding: 4px; border: 1px solid #e2e8f0; font-weight: bold; }"
    );
    matrixTable_->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(matrixTable_, &QTableWidget::cellClicked, this, &LiteratureMatrixWidget::onCellClicked);
    layout->addWidget(matrixTable_, 1);

    statsLabel_ = new QLabel("Load papers to build matrix");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void LiteratureMatrixWidget::setPapers(const QList<QPair<int, QString>>& papers) {
    papers_.clear();
    for (const auto& p : papers) papers_.append(p.second);
    if (criteria_.isEmpty()) addDefaultCriteria();
    refreshTable();
    updateStats();
}

void LiteratureMatrixWidget::setCriteria(const QStringList& criteria) {
    criteria_ = criteria;
    refreshTable();
}

void LiteratureMatrixWidget::setCell(int row, int col, const QString& value, const QColor& color) {
    MatrixCell c;
    c.row = row;
    c.col = col;
    c.value = value;
    c.color = color;
    cells_[{row, col}] = c;
}

void LiteratureMatrixWidget::addDefaultCriteria() {
    criteria_ = {"Method", "Dataset", "Year", "Metrics", "Code", "Baseline"};
}

void LiteratureMatrixWidget::autoFill() {
    QStringList methods = {"CNN", "RNN", "Transformer", "GAN", "RL", "GNN", "VAE", "Diffusion"};
    QStringList datasets = {"ImageNet", "COCO", "CIFAR-10", "MNIST", "SQuAD", "GLUE", "Custom"};
    QStringList years = {"2020", "2021", "2022", "2023", "2024", "2025", "2026"};
    QStringList metrics = {"Acc", "F1", "BLEU", "ROUGE", "mAP", "AUC", "Perplexity"};
    QStringList codeAvail = {"Yes", "No", "Partial"};

    for (int r = 0; r < papers_.size(); ++r) {
        for (int c = 0; c < criteria_.size(); ++c) {
            if (cells_.contains({r, c})) continue;
            QString val;
            QColor color(248, 250, 252);
            if (criteria_[c] == "Method") {
                val = methods[QRandomGenerator::global()->bounded(methods.size())];
                color = QColor(219, 234, 254);
            } else if (criteria_[c] == "Dataset") {
                val = datasets[QRandomGenerator::global()->bounded(datasets.size())];
                color = QColor(220, 252, 231);
            } else if (criteria_[c] == "Year") {
                val = years[QRandomGenerator::global()->bounded(years.size())];
                color = QColor(254, 243, 199);
            } else if (criteria_[c] == "Metrics") {
                val = metrics[QRandomGenerator::global()->bounded(metrics.size())];
                color = QColor(237, 233, 254);
            } else if (criteria_[c] == "Code") {
                val = codeAvail[QRandomGenerator::global()->bounded(codeAvail.size())];
                color = (val == "Yes") ? QColor(220, 252, 231) : QColor(254, 226, 226);
            } else if (criteria_[c] == "Baseline") {
                val = QString::number(QRandomGenerator::global()->bounded(100));
                color = QColor(240, 249, 255);
            }
            setCell(r, c, val, color);
        }
    }
    refreshTable();
    saveSettings();
    emit matrixUpdated(papers_.size(), criteria_.size());
}

MatrixCell LiteratureMatrixWidget::cell(int row, int col) const {
    return cells_.value({row, col});
}

void LiteratureMatrixWidget::onModeChanged(int) { refreshTable(); }
void LiteratureMatrixWidget::onCellClicked(int row, int col) {
    if (row < 0 || col < 0) return;
    QString paper = (row < papers_.size()) ? papers_[row] : "";
    QString criterion = (col < criteria_.size()) ? criteria_[col] : "";
    emit cellClicked(row, col, paper, criterion);
}

void LiteratureMatrixWidget::onExport() {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Matrix", "", "CSV (*.csv)");
    if (fileName.isEmpty()) return;
    QFile f(fileName);
    if (!f.open(QIODevice::WriteOnly)) return;
    QString header = "Paper," + criteria_.join(",") + "\n";
    f.write(header.toUtf8());
    for (int r = 0; r < papers_.size(); ++r) {
        QString line = "\"" + papers_[r] + "\"";
        for (int c = 0; c < criteria_.size(); ++c) {
            line += "," + cells_.value({r, c}).value;
        }
        f.write((line + "\n").toUtf8());
    }
}

void LiteratureMatrixWidget::onReset() {
    cells_.clear();
    refreshTable();
    saveSettings();
}

void LiteratureMatrixWidget::refreshTable() {
    matrixTable_->setRowCount(papers_.size());
    matrixTable_->setColumnCount(criteria_.size());

    QStringList headers;
    for (const auto& c : criteria_) headers << c;
    matrixTable_->setHorizontalHeaderLabels(headers);

    for (int r = 0; r < papers_.size(); ++r) {
        QTableWidgetItem* rowHeader = new QTableWidgetItem(papers_[r].left(25));
        rowHeader->setToolTip(papers_[r]);
        matrixTable_->setVerticalHeaderItem(r, rowHeader);

        for (int c = 0; c < criteria_.size(); ++c) {
            MatrixCell mc = cells_.value({r, c});
            auto* item = new QTableWidgetItem(mc.value);
            if (mc.color.isValid()) item->setBackground(mc.color);
            if (!mc.tooltip.isEmpty()) item->setToolTip(mc.tooltip);
            matrixTable_->setItem(r, c, item);
        }
    }
    matrixTable_->resizeColumnsToContents();
}

void LiteratureMatrixWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
}

void LiteratureMatrixWidget::updateStats() {
    statsLabel_->setText(QString("%1 papers x %2 criteria = %3 cells")
        .arg(papers_.size()).arg(criteria_.size()).arg(papers_.size() * criteria_.size()));
}

void LiteratureMatrixWidget::loadSettings() {
    papers_ = settings_.value("papers").toStringList();
    criteria_ = settings_.value("criteria").toStringList();
    int nRows = settings_.beginReadArray("cells");
    for (int i = 0; i < nRows; ++i) {
        settings_.setArrayIndex(i);
        int r = settings_.value("row").toInt();
        int c = settings_.value("col").toInt();
        MatrixCell mc;
        mc.row = r;
        mc.col = c;
        mc.value = settings_.value("value").toString();
        mc.color = QColor(settings_.value("color").toString());
        cells_[{r, c}] = mc;
    }
    settings_.endArray();
    if (!papers_.isEmpty() && !criteria_.isEmpty()) refreshTable();
    updateStats();
}

void LiteratureMatrixWidget::saveSettings() {
    settings_.setValue("papers", papers_);
    settings_.setValue("criteria", criteria_);
    settings_.beginWriteArray("cells");
    int idx = 0;
    for (auto it = cells_.begin(); it != cells_.end(); ++it) {
        settings_.setArrayIndex(idx++);
        settings_.setValue("row", it.key().first);
        settings_.setValue("col", it.key().second);
        settings_.setValue("value", it.value().value);
        settings_.setValue("color", it.value().color.name());
    }
    settings_.endArray();
}
