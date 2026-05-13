#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct NoteOrganizer2Entry {
    int id; QString note; QString category; QString notebook;
    qreal relevance; int words; bool pinned; QColor color;
};
class PaperNoteOrganizer2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperNoteOrganizer2(QWidget* parent = nullptr);
    void addEntry(const NoteOrganizer2Entry& entry);
    QList<NoteOrganizer2Entry> entries() const;
    int pinnedCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void noteOrganized(int id, qreal relevance);
private slots:
    void onOrganize();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawNoteView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<NoteOrganizer2Entry> entries_;
    QSettings settings_;
    QPushButton* organizeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
