#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BookmarkManagerEntry {
    int id; QString paper; QString category; QString location;
    qreal progress; int annotations; bool starred; QColor color;
};
class PaperBookmarkManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperBookmarkManager(QWidget* parent = nullptr);
    void addEntry(const BookmarkManagerEntry& entry);
    QList<BookmarkManagerEntry> entries() const;
    int starredCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bookmarkAdded(int id, qreal progress);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBookmarkView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BookmarkManagerEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
