#include "PaperAnnotationHighlighter.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFileDialog>
#include <QMessageBox>

PaperAnnotationHighlighter::PaperAnnotationHighlighter(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadSettings();
}

void PaperAnnotationHighlighter::setupUI() {
    auto* layout = new QVBoxLayout(this);

    paperLabel_ = new QLabel("Select a paper");
    paperLabel_->setStyleSheet("font-weight: bold; font-size: 13px;");
    layout->addWidget(paperLabel_);

    // Filter + color row
    auto* topRow = new QHBoxLayout();
    topRow->addWidget(new QLabel("Filter:"));
    filterCombo_ = new QComboBox();
    filterCombo_->addItems({"All", "Yellow", "Green", "Blue", "Red", "Purple", "Has Notes"});
    connect(filterCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperAnnotationHighlighter::onFilterChanged);
    topRow->addWidget(filterCombo_, 1);

    topRow->addWidget(new QLabel("Color:"));
    colorCombo_ = new QComboBox();
    colorCombo_->addItems({"Yellow", "Green", "Blue", "Red", "Purple"});
    connect(colorCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &PaperAnnotationHighlighter::onColorChanged);
    topRow->addWidget(colorCombo_);
    layout->addLayout(topRow);

    // Annotation list
    annList_ = new QListWidget();
    annList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(annList_, &QListWidget::itemClicked, this, &PaperAnnotationHighlighter::onAnnotationSelected);
    layout->addWidget(annList_, 1);

    // Highlighted text
    layout->addWidget(new QLabel("Highlighted text:"));
    textEdit_ = new QTextEdit();
    textEdit_->setMaximumHeight(60);
    textEdit_->setPlaceholderText("Paste or type highlighted text...");
    layout->addWidget(textEdit_);

    // Note
    layout->addWidget(new QLabel("Note:"));
    noteEdit_ = new QTextEdit();
    noteEdit_->setMaximumHeight(60);
    noteEdit_->setPlaceholderText("Add a note for this annotation...");
    layout->addWidget(noteEdit_);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add Annotation");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperAnnotationHighlighter::onAdd);
    btnRow->addWidget(addBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &PaperAnnotationHighlighter::onDelete);
    btnRow->addWidget(deleteBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperAnnotationHighlighter::onExport);
    btnRow->addWidget(exportBtn_);

    layout->addLayout(btnRow);

    statsLabel_ = new QLabel("0 annotations");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    layout->addWidget(statsLabel_);
}

void PaperAnnotationHighlighter::setPaper(int paperId, const QString& title) {
    currentPaperId_ = paperId;
    paperLabel_->setText(QString("Annotations: %1").arg(title));
    refreshList();
    updateStats();
}

void PaperAnnotationHighlighter::addAnnotation(const Annotation& ann) {
    Annotation a = ann;
    if (a.id < 0) a.id = nextId_++;
    if (a.timestamp == 0) a.timestamp = QDateTime::currentSecsSinceEpoch();
    if (a.paperId < 0) a.paperId = currentPaperId_;
    annotations_.append(a);
    nextId_ = qMax(nextId_, a.id + 1);
    refreshList();
    saveSettings();
    updateStats();
    emit annotationAdded(a.paperId, a.id);
}

void PaperAnnotationHighlighter::removeAnnotation(int annId) {
    annotations_.removeIf([annId](const Annotation& a) { return a.id == annId; });
    refreshList();
    saveSettings();
    updateStats();
    emit annotationRemoved(annId);
}

QList<Annotation> PaperAnnotationHighlighter::annotations() const { return annotations_; }

QList<Annotation> PaperAnnotationHighlighter::annotationsByColor(const QString& color) const {
    QList<Annotation> result;
    for (const auto& a : annotations_) {
        if (a.color == color) result.append(a);
    }
    return result;
}

void PaperAnnotationHighlighter::exportAnnotations(const QString& format) {
    QString fileName = QFileDialog::getSaveFileName(this, "Export Annotations", "",
        format == "json" ? "JSON Files (*.json)" : "Text Files (*.txt)");
    if (fileName.isEmpty()) return;

    if (format == "json") {
        QJsonArray arr;
        for (const auto& a : annotations_) {
            QJsonObject obj;
            obj["id"] = a.id;
            obj["paperId"] = a.paperId;
            obj["text"] = a.text;
            obj["note"] = a.note;
            obj["color"] = a.color;
            obj["timestamp"] = static_cast<qint64>(a.timestamp);
            arr.append(obj);
        }
        QFile f(fileName);
        if (f.open(QIODevice::WriteOnly)) {
            f.write(QJsonDocument(arr).toJson());
        }
    } else {
        QFile f(fileName);
        if (f.open(QIODevice::WriteOnly)) {
            for (const auto& a : annotations_) {
                QString line = QString("[%1] %2 | %3 | %4\n  Note: %5\n\n")
                    .arg(a.color, a.text.left(80), a.note,
                         QDateTime::fromSecsSinceEpoch(a.timestamp).toString("yyyy-MM-dd HH:mm"),
                         a.note);
                f.write(line.toUtf8());
            }
        }
    }
}

void PaperAnnotationHighlighter::onAdd() {
    if (currentPaperId_ < 0 || textEdit_->toPlainText().trimmed().isEmpty()) return;
    Annotation a;
    a.paperId = currentPaperId_;
    a.text = textEdit_->toPlainText();
    a.note = noteEdit_->toPlainText();
    a.color = colorCombo_->currentText();
    addAnnotation(a);
    textEdit_->clear();
    noteEdit_->clear();
}

void PaperAnnotationHighlighter::onDelete() {
    if (selectedId_ < 0) return;
    removeAnnotation(selectedId_);
    selectedId_ = -1;
}

void PaperAnnotationHighlighter::onAnnotationSelected() {
    auto* item = annList_->currentItem();
    if (!item) return;
    selectedId_ = item->data(Qt::UserRole).toInt();
    for (const auto& a : annotations_) {
        if (a.id == selectedId_) {
            textEdit_->setPlainText(a.text);
            noteEdit_->setPlainText(a.note);
            int ci = colorCombo_->findText(a.color);
            if (ci >= 0) colorCombo_->setCurrentIndex(ci);
            emit annotationClicked(a.id);
            break;
        }
    }
}

void PaperAnnotationHighlighter::onColorChanged(int) {}
void PaperAnnotationHighlighter::onFilterChanged(int) { refreshList(); }

void PaperAnnotationHighlighter::onExport() { exportAnnotations("json"); }

void PaperAnnotationHighlighter::refreshList() {
    annList_->clear();
    int filter = filterCombo_->currentIndex();
    QMap<QString, QColor> colorMap = {
        {"Yellow", QColor(250, 204, 21)}, {"Green", QColor(34, 197, 94)},
        {"Blue", QColor(59, 130, 246)}, {"Red", QColor(239, 68, 68)},
        {"Purple", QColor(168, 85, 247)}
    };

    for (const auto& a : annotations_) {
        if (a.paperId != currentPaperId_ && currentPaperId_ >= 0) continue;
        if (filter >= 1 && filter <= 5 && a.color != filterCombo_->itemText(filter)) continue;
        if (filter == 6 && a.note.isEmpty()) continue;

        QString display = QString("[%1] %2%3")
            .arg(a.color,
                 a.text.left(60),
                 a.note.isEmpty() ? "" : " *");
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, a.id);
        if (colorMap.contains(a.color)) {
            item->setForeground(colorMap[a.color]);
        }
        annList_->addItem(item);
    }
}

void PaperAnnotationHighlighter::updateStats() {
    int total = 0;
    for (const auto& a : annotations_) {
        if (currentPaperId_ < 0 || a.paperId == currentPaperId_) total++;
    }
    statsLabel_->setText(QString("%1 annotations").arg(total));
}

void PaperAnnotationHighlighter::loadSettings() {
    QSettings settings("PaperCrawler", "Annotations");
    QByteArray data = settings.value("annotations").toByteArray();
    if (data.isEmpty()) return;
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        Annotation a;
        a.id = obj["id"].toInt();
        a.paperId = obj["paperId"].toInt();
        a.text = obj["text"].toString();
        a.note = obj["note"].toString();
        a.color = obj["color"].toString();
        a.page = obj["page"].toInt();
        a.startPos = obj["startPos"].toInt();
        a.endPos = obj["endPos"].toInt();
        a.timestamp = obj["timestamp"].toInteger();
        annotations_.append(a);
        nextId_ = qMax(nextId_, a.id + 1);
    }
    refreshList();
}

void PaperAnnotationHighlighter::saveSettings() {
    QJsonArray arr;
    for (const auto& a : annotations_) {
        QJsonObject obj;
        obj["id"] = a.id;
        obj["paperId"] = a.paperId;
        obj["text"] = a.text;
        obj["note"] = a.note;
        obj["color"] = a.color;
        obj["page"] = a.page;
        obj["startPos"] = a.startPos;
        obj["endPos"] = a.endPos;
        obj["timestamp"] = static_cast<qint64>(a.timestamp);
        arr.append(obj);
    }
    QSettings settings("PaperCrawler", "Annotations");
    settings.setValue("annotations", QJsonDocument(arr).toJson(QJsonDocument::Compact));
}
