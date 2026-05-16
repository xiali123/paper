#include "tools/QuickNoteWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QFile>
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>

QuickNoteWidget::QuickNoteWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadNotes();
}

void QuickNoteWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Input area
    auto* inputRow = new QHBoxLayout();
    inputEdit_ = new QPlainTextEdit();
    inputEdit_->setMaximumHeight(80);
    inputEdit_->setPlaceholderText("Quick note...");
    inputRow->addWidget(inputEdit_, 1);

    auto* addBtn = new QPushButton("Add");
    addBtn->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; border-radius: 6px; "
        "padding: 8px 16px; font-weight: bold; }"
        "QPushButton:hover { background: #2563eb; }"
    );
    connect(addBtn, &QPushButton::clicked, this, &QuickNoteWidget::onAddNote);
    inputRow->addWidget(addBtn);

    layout->addLayout(inputRow);

    // Color selector
    auto* colorRow = new QHBoxLayout();
    colorRow->addWidget(new QLabel("Color:"));
    QStringList colors = {"#ffffff", "#fef3c7", "#dbeafe", "#dcfce7", "#fce7f3", "#f3e8ff"};
    for (const auto& c : colors) {
        auto* btn = new QPushButton();
        btn->setFixedSize(24, 24);
        btn->setStyleSheet(QString("background: %1; border: 1px solid #999; border-radius: 12px;").arg(c));
        btn->setCheckable(true);
        if (c == "#ffffff") btn->setChecked(true);
        connect(btn, &QPushButton::clicked, this, [this, btn, colorRow]() {
            for (int i = 0; i < colorRow->count(); ++i) {
                auto* w = colorRow->itemAt(i)->widget();
                if (auto* b = qobject_cast<QPushButton*>(w)) b->setChecked(false);
            }
            btn->setChecked(true);
        });
        colorRow->addWidget(btn);
    }
    colorRow->addStretch();
    layout->addLayout(colorRow);

    // Notes list
    auto* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setStyleSheet("QScrollArea { border: none; }");

    auto* scrollContent = new QWidget();
    notesLayout_ = new QVBoxLayout(scrollContent);
    notesLayout_->setAlignment(Qt::AlignTop);
    notesLayout_->setSpacing(8);
    notesLayout_->addStretch();
    scroll->setWidget(scrollContent);

    layout->addWidget(scroll, 1);
}

void QuickNoteWidget::onAddNote() {
    QString text = inputEdit_->toPlainText().trimmed();
    if (text.isEmpty()) return;

    QuickNote note;
    note.id = nextId_++;
    note.text = text;
    note.timestamp = QDateTime::currentSecsSinceEpoch();
    note.color = "#ffffff";

    // Get selected color
    auto* scrollContent = notesLayout_->parentWidget();
    auto* colorRow = scrollContent->findChild<QHBoxLayout*>();
    Q_UNUSED(colorRow);

    notes_.prepend(note);
    inputEdit_->clear();
    refreshList();
    saveNotes();
    emit noteCreated(note);
}

void QuickNoteWidget::onDeleteNote(int noteId) {
    notes_.removeIf([noteId](const QuickNote& n) { return n.id == noteId; });
    refreshList();
    saveNotes();
    emit noteDeleted(noteId);
}

void QuickNoteWidget::onColorChanged(int noteId, const QString& color) {
    for (auto& n : notes_) {
        if (n.id == noteId) {
            n.color = color;
            break;
        }
    }
    refreshList();
    saveNotes();
}

void QuickNoteWidget::refreshList() {
    while (notesLayout_->count() > 1) {
        auto* item = notesLayout_->takeAt(0);
        delete item->widget();
        delete item;
    }

    for (const auto& note : notes_) {
        auto* card = createNoteCard(note);
        notesLayout_->insertWidget(notesLayout_->count() - 1, card);
    }
}

QWidget* QuickNoteWidget::createNoteCard(const QuickNote& note) {
    auto* card = new QWidget();
    QString border = note.color == "#ffffff" ? "palette(mid)" : note.color;
    card->setStyleSheet(QString(
        "QWidget { background: %1; border: 1px solid %2; border-radius: 8px; padding: 8px; }"
    ).arg(note.color, border));

    auto* layout = new QVBoxLayout(card);
    layout->setContentsMargins(8, 6, 8, 6);
    layout->setSpacing(4);

    auto* textLabel = new QLabel(note.text);
    textLabel->setWordWrap(true);
    textLabel->setStyleSheet("border: none; background: transparent;");
    layout->addWidget(textLabel);

    auto* bottomRow = new QHBoxLayout();

    auto* timeLabel = new QLabel(QDateTime::fromSecsSinceEpoch(note.timestamp).toString("MM/dd HH:mm"));
    timeLabel->setStyleSheet("font-size: 10px; color: #666; border: none; background: transparent;");
    bottomRow->addWidget(timeLabel);
    bottomRow->addStretch();

    auto* deleteBtn = new QPushButton("X");
    deleteBtn->setFixedSize(20, 20);
    deleteBtn->setStyleSheet(
        "QPushButton { background: transparent; color: #dc2626; border: none; font-size: 12px; font-weight: bold; }"
        "QPushButton:hover { background: #fee2e2; border-radius: 10px; }"
    );
    connect(deleteBtn, &QPushButton::clicked, this, [this, id = note.id]() {
        onDeleteNote(id);
    });
    bottomRow->addWidget(deleteBtn);

    layout->addLayout(bottomRow);

    return card;
}

void QuickNoteWidget::loadNotes() {
    QString path = filePath();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray arr = doc.array();
    for (const auto& item : arr) {
        QJsonObject obj = item.toObject();
        QuickNote note;
        note.id = obj["id"].toInt();
        note.text = obj["text"].toString();
        note.timestamp = obj["timestamp"].toInteger();
        note.color = obj["color"].toString("#ffffff");
        notes_.append(note);
        nextId_ = qMax(nextId_, note.id + 1);
    }

    // Trim to max
    while (notes_.size() > MAX_NOTES) notes_.removeLast();
    refreshList();
}

void QuickNoteWidget::saveNotes() {
    QString path = filePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QJsonArray arr;
    for (const auto& note : notes_) {
        QJsonObject obj;
        obj["id"] = note.id;
        obj["text"] = note.text;
        obj["timestamp"] = note.timestamp;
        obj["color"] = note.color;
        arr.append(obj);
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(arr).toJson(QJsonDocument::Compact));
    }
}

QString QuickNoteWidget::filePath() const {
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/quick_notes.json";
}
