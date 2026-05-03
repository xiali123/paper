#include "AnnotationWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDateTime>

AnnotationWidget::AnnotationWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void AnnotationWidget::setupUI() {
    auto* layout = new QHBoxLayout(this);

    // Left: annotation list
    auto* leftPanel = new QVBoxLayout();

    auto* header = new QLabel("Annotations");
    header->setStyleSheet("font-weight: bold; font-size: 13px;");
    leftPanel->addWidget(header);

    // Filter row
    auto* filterRow = new QHBoxLayout();
    filterRow->addWidget(new QLabel("Category:"));
    categoryCombo_ = new QComboBox();
    categoryCombo_->addItems({"All", "Important", "Question", "Insight", "Method", "Result"});
    connect(categoryCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AnnotationWidget::onCategoryFilterChanged);
    filterRow->addWidget(categoryCombo_, 1);
    leftPanel->addLayout(filterRow);

    listWidget_ = new QListWidget();
    listWidget_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; border-bottom: 1px solid palette(mid); }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    listWidget_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    connect(listWidget_, &QListWidget::itemClicked, this, &AnnotationWidget::onItemClicked);
    leftPanel->addWidget(listWidget_, 1);

    countLabel_ = new QLabel("0 annotations");
    leftPanel->addWidget(countLabel_);

    auto* listBtnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("background: #3b82f6; color: white; border-radius: 4px; padding: 4px 12px;");
    connect(addBtn_, &QPushButton::clicked, this, &AnnotationWidget::onAddAnnotation);
    listBtnRow->addWidget(addBtn_);

    editBtn_ = new QPushButton("Edit");
    connect(editBtn_, &QPushButton::clicked, this, &AnnotationWidget::onEditAnnotation);
    listBtnRow->addWidget(editBtn_);

    deleteBtn_ = new QPushButton("Delete");
    deleteBtn_->setStyleSheet("color: #dc2626;");
    connect(deleteBtn_, &QPushButton::clicked, this, &AnnotationWidget::onDeleteAnnotation);
    listBtnRow->addWidget(deleteBtn_);

    leftPanel->addLayout(listBtnRow);

    layout->addLayout(leftPanel, 1);

    // Right: note editor
    auto* rightPanel = new QVBoxLayout();
    rightPanel->addWidget(new QLabel("Note:"));
    noteEdit_ = new QPlainTextEdit();
    noteEdit_->setPlaceholderText("Write annotation note here...");
    noteEdit_->setStyleSheet(
        "QPlainTextEdit { border: 1px solid palette(mid); border-radius: 4px; padding: 8px; }"
    );
    rightPanel->addWidget(noteEdit_, 1);
    layout->addLayout(rightPanel, 1);
}

void AnnotationWidget::setPaperId(int paperId) {
    paperId_ = paperId;
}

void AnnotationWidget::setAnnotations(const QList<Annotation>& annotations) {
    annotations_ = annotations;
    nextId_ = 1;
    for (const auto& a : annotations_) nextId_ = qMax(nextId_, a.id + 1);
    refreshList();
}

QList<Annotation> AnnotationWidget::annotations() const {
    return annotations_;
}

void AnnotationWidget::addAnnotation(const Annotation& ann) {
    Annotation a = ann;
    if (a.id < 0) a.id = nextId_++;
    a.paperId = paperId_;
    a.createdAt = QDateTime::currentSecsSinceEpoch();
    if (a.color == QColor()) a.color = categoryColor(a.category);
    annotations_.append(a);
    refreshList();
    emit annotationAdded(a);
}

void AnnotationWidget::removeAnnotation(int annId) {
    annotations_.removeIf([annId](const Annotation& a) { return a.id == annId; });
    refreshList();
    emit annotationRemoved(annId);
}

void AnnotationWidget::updateAnnotation(int annId, const Annotation& ann) {
    for (auto& a : annotations_) {
        if (a.id == annId) {
            a = ann;
            a.id = annId;
            break;
        }
    }
    refreshList();
    emit annotationUpdated(annId);
}

QList<Annotation> AnnotationWidget::filterByCategory(const QString& category) const {
    QList<Annotation> result;
    for (const auto& a : annotations_) {
        if (a.category == category) result.append(a);
    }
    return result;
}

QList<Annotation> AnnotationWidget::filterByPage(int page) const {
    QList<Annotation> result;
    for (const auto& a : annotations_) {
        if (a.page == page) result.append(a);
    }
    return result;
}

void AnnotationWidget::onAddAnnotation() {
    Annotation ann;
    ann.category = "important";
    ann.text = noteEdit_->toPlainText().trimmed();
    ann.color = categoryColor(ann.category);
    if (ann.text.isEmpty()) {
        ann.text = "New annotation";
    }
    addAnnotation(ann);
    noteEdit_->clear();
}

void AnnotationWidget::onDeleteAnnotation() {
    if (selectedAnnId_ < 0) return;
    removeAnnotation(selectedAnnId_);
    selectedAnnId_ = -1;
}

void AnnotationWidget::onEditAnnotation() {
    if (selectedAnnId_ < 0) return;
    for (auto& a : annotations_) {
        if (a.id == selectedAnnId_) {
            a.text = noteEdit_->toPlainText().trimmed();
            refreshList();
            emit annotationUpdated(selectedAnnId_);
            break;
        }
    }
}

void AnnotationWidget::onCategoryFilterChanged(int index) {
    currentFilter_ = (index <= 0) ? "" : categoryCombo_->itemText(index).toLower();
    refreshList();
}

void AnnotationWidget::onItemClicked(QListWidgetItem* item) {
    if (!item) return;
    int annId = item->data(Qt::UserRole).toInt();
    selectedAnnId_ = annId;
    for (const auto& a : annotations_) {
        if (a.id == annId) {
            noteEdit_->setPlainText(a.text);
            break;
        }
    }
    emit annotationClicked(annId);
}

void AnnotationWidget::refreshList() {
    listWidget_->clear();
    for (const auto& a : annotations_) {
        if (!currentFilter_.isEmpty() && a.category != currentFilter_) continue;

        QString icon = categoryIcon(a.category);
        QString time = QDateTime::fromSecsSinceEpoch(a.createdAt).toString("MM/dd HH:mm");
        QString display = QString("%1 [%2] %3\n%4 — Page %5")
            .arg(icon, a.category, a.text.left(60), time)
            .arg(a.page);

        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, a.id);
        item->setForeground(a.color.isValid() ? a.color : Qt::black);
        listWidget_->addItem(item);
    }
    countLabel_->setText(QString("%1 annotation(s)").arg(listWidget_->count()));
}

QColor AnnotationWidget::categoryColor(const QString& category) const {
    static QMap<QString, QColor> colors = {
        {"important", QColor(239, 68, 68)},
        {"question", QColor(245, 158, 11)},
        {"insight", QColor(16, 185, 129)},
        {"method", QColor(59, 130, 246)},
        {"result", QColor(139, 92, 246)},
    };
    return colors.value(category, QColor(100, 116, 139));
}

QString AnnotationWidget::categoryIcon(const QString& category) const {
    static QMap<QString, QString> icons = {
        {"important", "!!"}, {"question", "??"},
        {"insight", "**"}, {"method", ">>"}, {"result", "=>"},
    };
    return icons.value(category, "--");
}
