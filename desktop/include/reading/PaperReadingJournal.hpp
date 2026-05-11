#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct JournalEntry {
    int id;
    QString title;
    QString reflection;
    QString category;
    QString mood;
    qreal rating;
    QString date;
    bool bookmarked;
    QColor color;
};

class PaperReadingJournal : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingJournal(QWidget* parent = nullptr);
    void addEntry(const JournalEntry& entry);
    QList<JournalEntry> entries() const;
    int bookmarkedCount() const;
    qreal avgRating() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void journalCreated(int id, qreal rating);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawJournalList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<JournalEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
