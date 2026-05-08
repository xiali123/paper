#include "visualization/PaperFigureExtractor.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

PaperFigureExtractor::PaperFigureExtractor(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperFigureExtractor::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Type:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Figure", "Table", "Algorithm", "Chart", "Diagram"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperFigureExtractor::onFilterChanged);
    filterRow->addWidget(filterCombo_, 1);
    layout->addLayout(filterRow);

    auto* splitter = new QSplitter(Qt::Horizontal);

    figureList_ = new QListWidget();
    figureList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(figureList_, &QListWidget::itemClicked, this, &PaperFigureExtractor::onFigureSelected);
    splitter->addWidget(figureList_);

    auto* rightPanel = new QWidget();
    auto* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->setContentsMargins(0, 0, 0, 0);

    auto* formRow1 = new QHBoxLayout();
    formRow1->addWidget(new QLabel("Number:"));
    numberEdit_ = new QLineEdit();
    numberEdit_->setMaximumWidth(50);
    numberEdit_->setPlaceholderText("#");
    formRow1->addWidget(numberEdit_);

    formRow1->addWidget(new QLabel("Type:"));
    typeCombo_ = new QComboBox();
    typeCombo_->addItems({"Figure", "Table", "Algorithm", "Chart", "Diagram"});
    formRow1->addWidget(typeCombo_);

    formRow1->addWidget(new QLabel("Caption:"));
    captionEdit_ = new QLineEdit();
    captionEdit_->setPlaceholderText("Figure caption...");
    formRow1->addWidget(captionEdit_, 1);
    rightLayout->addLayout(formRow1);

    rightLayout->addWidget(new QLabel("Description:"));
    descEdit_ = new QTextEdit();
    descEdit_->setMaximumHeight(80);
    descEdit_->setPlaceholderText("Describe the figure content, key findings, data shown...");
    rightLayout->addWidget(descEdit_);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Figure");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperFigureExtractor::onAdd);
    btnRow->addWidget(addBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperFigureExtractor::onDelete);
    btnRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export Catalog");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperFigureExtractor::onExport);
    btnRow->addWidget(exportBtn_);
    rightLayout->addLayout(btnRow);

    splitter->addWidget(rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);
    layout->addWidget(splitter, 1);

    statsLabel_ = new QLabel("0 figures");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperFigureExtractor::setPaper(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Figures: %1").arg(title));
    refreshList();
    updateStats();
}

void PaperFigureExtractor::addFigure(const FigureEntry& fig) {
    FigureEntry f = fig;
    if (f.id < 0) f.id = nextId_++;
    if (f.paperId < 0) f.paperId = currentPaperId_;
    figures_.append(f);
    nextId_ = qMax(nextId_, f.id + 1);
    refreshList();
    saveSettings();
    updateStats();
}

void PaperFigureExtractor::removeFigure(int figId) {
    figures_.removeIf([figId](const FigureEntry& f) { return f.id == figId; });
    if (selectedId_ == figId) selectedId_ = -1;
    refreshList();
    saveSettings();
    updateStats();
}

QList<FigureEntry> PaperFigureExtractor::figures() const { return figures_; }

QList<FigureEntry> PaperFigureExtractor::figuresByType(const QString& type) const {
    QList<FigureEntry> result;
    for (const auto& f : figures_) {
        if (f.figureType == type) result.append(f);
    }
    return result;
}

void PaperFigureExtractor::exportCatalog(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly)) return;
    QJsonArray arr;
    for (const auto& fig : figures_) {
        QJsonObject obj;
        obj["id"] = fig.id;
        obj["paperId"] = fig.paperId;
        obj["number"] = fig.number;
        obj["figureType"] = fig.figureType;
        obj["caption"] = fig.caption;
        obj["description"] = fig.description;
        obj["tags"] = QJsonArray::fromStringList(fig.tags);
        arr.append(obj);
    }
    f.write(QJsonDocument(arr).toJson());
    emit figureExported(figures_.size());
}

void PaperFigureExtractor::onAdd() {
    if (captionEdit_->text().trimmed().isEmpty()) return;
    FigureEntry fig;
    fig.paperId = currentPaperId_;
    fig.number = numberEdit_->text().toInt();
    if (fig.number <= 0) fig.number = figures_.size() + 1;
    fig.figureType = typeCombo_->currentText().toLower();
    fig.caption = captionEdit_->text().trimmed();
    fig.description = descEdit_->toPlainText();
    addFigure(fig);
    captionEdit_->clear();
    descEdit_->clear();
    numberEdit_->clear();
}

void PaperFigureExtractor::onDelete() {
    if (selectedId_ < 0) return;
    removeFigure(selectedId_);
}

void PaperFigureExtractor::onExport() {
    QString path = QFileDialog::getSaveFileName(this, "Export Figure Catalog", "", "JSON (*.json)");
    if (path.isEmpty()) return;
    exportCatalog(path);
}

void PaperFigureExtractor::onFilterChanged(int) { refreshList(); }

void PaperFigureExtractor::onFigureSelected() {
    auto* item = figureList_->currentItem();
    if (!item) return;
    selectedId_ = item->data(Qt::UserRole).toInt();
    for (const auto& f : figures_) {
        if (f.id == selectedId_) {
            numberEdit_->setText(QString::number(f.number));
            captionEdit_->setText(f.caption);
            descEdit_->setPlainText(f.description);
            int ti = typeCombo_->findText(f.figureType, Qt::MatchFixedString);
            if (ti >= 0) typeCombo_->setCurrentIndex(ti);
            emit figureClicked(f.id, f.caption);
            break;
        }
    }
}

void PaperFigureExtractor::refreshList() {
    figureList_->clear();
    QString filter = filterCombo_->currentText().toLower();
    QMap<QString, QColor> typeColors = {
        {"figure", QColor(59,130,246)}, {"table", QColor(16,185,129)},
        {"algorithm", QColor(245,158,11)}, {"chart", QColor(139,92,246)}, {"diagram", QColor(236,72,153)}
    };

    for (const auto& f : figures_) {
        if (filter != "all" && f.figureType != filter) continue;
        QString display = QString("%1 %2: %3")
            .arg(f.figureType, QString::number(f.number), f.caption.left(40));
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, f.id);
        if (typeColors.contains(f.figureType)) item->setForeground(typeColors[f.figureType]);
        figureList_->addItem(item);
    }
}

void PaperFigureExtractor::updateStats() {
    QMap<QString, int> counts;
    for (const auto& f : figures_) counts[f.figureType]++;
    QStringList parts;
    for (auto it = counts.begin(); it != counts.end(); ++it) {
        parts << QString("%1: %2").arg(it.key()).arg(it.value());
    }
    statsLabel_->setText(QString("%1 figures | %2").arg(figures_.size()).arg(parts.join(", ")));
}

void PaperFigureExtractor::loadSettings() {
    QSettings settings("PaperCrawler", "FigureExtractor");
    QByteArray data = settings.value("figures").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        FigureEntry f;
        f.id = obj["id"].toInt();
        f.paperId = obj["paperId"].toInt();
        f.number = obj["number"].toInt();
        f.figureType = obj["figureType"].toString();
        f.caption = obj["caption"].toString();
        f.description = obj["description"].toString();
        f.filePath = obj["filePath"].toString();
        QJsonArray tags = obj["tags"].toArray();
        for (const auto& t : tags) f.tags.append(t.toString());
        figures_.append(f);
        nextId_ = qMax(nextId_, f.id + 1);
    }
    refreshList();
    updateStats();
}

void PaperFigureExtractor::saveSettings() {
    QJsonArray arr;
    for (const auto& f : figures_) {
        QJsonObject obj;
        obj["id"] = f.id;
        obj["paperId"] = f.paperId;
        obj["number"] = f.number;
        obj["figureType"] = f.figureType;
        obj["caption"] = f.caption;
        obj["description"] = f.description;
        obj["filePath"] = f.filePath;
        obj["tags"] = QJsonArray::fromStringList(f.tags);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "FigureExtractor");
    settings.setValue("figures", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
