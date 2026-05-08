#pragma once

#include <QDialog>
#include <QTextEdit>
#include <QLabel>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QList>

struct PaperNote {
    int id{0};
    int paperId{0};
    QString content;
    QString highlight;   // highlighted text from paper
    int page{0};
    QString createdAt;

    static PaperNote fromJson(const QJsonObject& json);
    QJsonObject toJson() const;
};

class ApiManager;

class PaperNotesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaperNotesDialog(int paperId, const QString& paperTitle,
                               ApiManager* apiManager, QWidget* parent = nullptr);

private slots:
    void onAddNote();
    void onEditNote();
    void onDeleteNote();
    void onNoteSelected(int row);

private:
    void setupUI();
    void loadNotes();
    void refreshList();

    int paperId_;
    QString paperTitle_;
    ApiManager* apiManager_{nullptr};

    QListWidget* notesList_{nullptr};
    QTextEdit* contentEdit_{nullptr};
    QLineEdit* highlightEdit_{nullptr};
    QLineEdit* pageEdit_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* editBtn_{nullptr};
    QPushButton* deleteBtn_{nullptr};

    QList<PaperNote> notes_;
    int selectedIdx_{-1};
};
