#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct NoteSearchEntry {
    int id; QString query; QString category; QString paper;
    qreal relevance; int results; bool exact; QColor color;
};
class PaperReadingNoteSearcher : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingNoteSearcher(QWidget* parent = nullptr);
    void addEntry(const NoteSearchEntry& entry);
    QList<NoteSearchEntry> entries() const;
    int exactCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void searchComplete(int id, qreal relevance);
private slots:
    void onSearch();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawResultList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<NoteSearchEntry> entries_;
    QSettings settings_;
    QPushButton* searchBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
