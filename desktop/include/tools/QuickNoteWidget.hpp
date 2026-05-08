#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>
#include <QTimer>
#include <QJsonArray>

struct QuickNote {
    int id{-1};
    QString text;
    qint64 timestamp{0};
    QString color;
};

class QuickNoteWidget : public QWidget {
    Q_OBJECT

public:
    explicit QuickNoteWidget(QWidget* parent = nullptr);

    void loadNotes();
    void saveNotes();

signals:
    void noteCreated(const QuickNote& note);
    void noteDeleted(int noteId);

private slots:
    void onAddNote();
    void onDeleteNote(int noteId);
    void onColorChanged(int noteId, const QString& color);

private:
    void setupUI();
    void refreshList();
    QWidget* createNoteCard(const QuickNote& note);
    QString filePath() const;

    QPlainTextEdit* inputEdit_{nullptr};
    QVBoxLayout* notesLayout_{nullptr};
    QList<QuickNote> notes_;
    int nextId_{1};

    static constexpr int MAX_NOTES = 50;
};
