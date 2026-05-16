#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BookmarkVault2Entry {
    int id; QString title; QString category; QString folder;
    qreal relevance; int visits; bool pinned; QColor color;
};
class PaperBookmarkVault2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperBookmarkVault2(QWidget* parent = nullptr);
    void addEntry(const BookmarkVault2Entry& entry);
    QList<BookmarkVault2Entry> entries() const;
    int pinnedCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bookmarkPinned(int id, qreal relevance);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVaultView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BookmarkVault2Entry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
