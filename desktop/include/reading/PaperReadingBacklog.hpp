#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BacklogEntry {
    int id; QString paper; QString category; QString priority;
    int pages; qreal difficulty; QString addedDate; bool urgent; QColor color;
};
class PaperReadingBacklog : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingBacklog(QWidget* parent = nullptr);
    void addEntry(const BacklogEntry& entry);
    QList<BacklogEntry> entries() const;
    int urgentCount() const;
    qreal avgDifficulty() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void backlogAdded(int id, qreal difficulty);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBacklogList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BacklogEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
