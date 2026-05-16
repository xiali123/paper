#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct NoteEntry {
    int id;
    QString title;
    QString category;
    QString tag;
    QString date;
    int words;
    qreal importance;
    bool pinned;
    QColor color;
};

class PaperNoteOrganizer : public QWidget {
    Q_OBJECT
public:
    explicit PaperNoteOrganizer(QWidget* parent = nullptr);
    void addEntry(const NoteEntry& entry);
    QList<NoteEntry> entries() const;
    int pinnedCount() const;
    qreal avgImportance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void noteCreated(int id, qreal importance);
private slots:
    void onCreate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawNoteList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<NoteEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
