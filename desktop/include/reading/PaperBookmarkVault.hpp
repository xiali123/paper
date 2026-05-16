#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BookmarkVaultEntry {
    int id; QString title; QString category; QString location;
    qreal relevance; int visits; bool pinned; QColor color;
};
class PaperBookmarkVault : public QWidget {
    Q_OBJECT
public:
    explicit PaperBookmarkVault(QWidget* parent = nullptr);
    void addEntry(const BookmarkVaultEntry& entry);
    QList<BookmarkVaultEntry> entries() const;
    int pinnedCount() const;
    qreal avgRelevance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void bookmarkAccessed(int id, qreal relevance);
private slots:
    void onSave();
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
    QList<BookmarkVaultEntry> entries_;
    QSettings settings_;
    QPushButton* saveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
