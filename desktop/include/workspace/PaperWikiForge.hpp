#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct WikiForgeEntry {
    int id; QString page; QString category; QString editor;
    qreal edits; int revisions; bool featured; QColor color;
};
class PaperWikiForge : public QWidget {
    Q_OBJECT
public:
    explicit PaperWikiForge(QWidget* parent = nullptr);
    void addEntry(const WikiForgeEntry& entry);
    QList<WikiForgeEntry> entries() const;
    int featuredCount() const;
    qreal avgEdits() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void pageForged(int id, qreal edits);
private slots:
    void onForge();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawForgeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<WikiForgeEntry> entries_;
    QSettings settings_;
    QPushButton* forgeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
