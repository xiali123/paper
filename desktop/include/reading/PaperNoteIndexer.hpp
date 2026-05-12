#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct NoteIndexEntry {
    int id; QString note; QString category; QString keyword;
    qreal relevance; int matches; bool indexed; QColor color;
};
class PaperNoteIndexer : public QWidget {
    Q_OBJECT
public:
    explicit PaperNoteIndexer(QWidget* parent = nullptr);
    void addEntry(const NoteIndexEntry& entry);
    QList<NoteIndexEntry> entries() const;
    int indexedCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void noteIndexed(int id, qreal relevance);
private slots:
    void onIndex();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawIndexView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<NoteIndexEntry> entries_;
    QSettings settings_;
    QPushButton* indexBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
