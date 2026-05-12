#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BookmarkEntry {
    int id; QString title; QString category; QString paper;
    qreal relevance; int visits; bool starred; QColor color;
};
class PaperReadingBookmarks : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingBookmarks(QWidget* parent = nullptr);
    void addEntry(const BookmarkEntry& entry);
    QList<BookmarkEntry> entries() const;
    int starredCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bookmarkAdded(int id, qreal relevance);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBookmarkList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BookmarkEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
