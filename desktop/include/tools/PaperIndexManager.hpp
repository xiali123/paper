#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct IndexEntry {
    int id; QString index; QString category; QString type;
    qreal size; int documents; bool active; QColor color;
};
class PaperIndexManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperIndexManager(QWidget* parent = nullptr);
    void addEntry(const IndexEntry& entry);
    QList<IndexEntry> entries() const;
    int activeCount() const;
    qreal totalSize() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void indexRebuilt(int id, qreal size);
private slots:
    void onRebuild();
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
    QList<IndexEntry> entries_;
    QSettings settings_;
    QPushButton* rebuildBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
