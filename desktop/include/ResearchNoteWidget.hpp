#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct ResearchNote {
    int id{-1};
    QString title;
    QString content;
    QString category; // "idea", "finding", "method", "question", "reference"
    QColor color;
    QDate createdDate;
    QString paperTitle;
    int paperId{-1};
    QStringList tags;
};

class ResearchNoteWidget : public QWidget {
    Q_OBJECT

public:
    explicit ResearchNoteWidget(QWidget* parent = nullptr);

    void addNote(const ResearchNote& note);
    QList<ResearchNote> notes() const;
    QMap<QString, int> categoryCounts() const;
    QStringList allTags() const;

signals:
    void noteClicked(int noteId);
    void notesChanged(int count);

private slots:
    void onAdd();
    void onCategoryFilter(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawNoteCards(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawTimeline(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QComboBox* categoryCombo_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<ResearchNote> notes_;
    int selectedNote_{-1};
    QSettings settings_;
};
