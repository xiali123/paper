#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>
#include <QDate>

struct ReadingNote {
    int id{-1};
    QString title;
    QString paperTitle;
    QString category; // "summary", "question", "insight", "quote"
    QString content;
    QDate date;
    int priority{0};
    QStringList tags;
    QColor color;
};

class PaperReadingNotesOrganizer : public QWidget {
    Q_OBJECT

public:
    explicit PaperReadingNotesOrganizer(QWidget* parent = nullptr);

    void addNote(const ReadingNote& note);
    QList<ReadingNote> notes() const;
    QMap<QString, int> categoryCounts() const;
    int notesToday() const;
    int totalTags() const;

signals:
    void noteAdded(int noteId, const QString& category);
    void notesOrganized(int count);

private slots:
    void onAdd();
    void onFilterChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawNoteCards(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* filterCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ReadingNote> notes_;
    QSettings settings_;
};
