#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingNexus2Entry {
    int id; QString paper; QString category; QString technique;
    qreal comprehension; int sessions; bool mastered; QColor color;
};
class PaperReadingNexus2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingNexus2(QWidget* parent = nullptr);
    void addEntry(const ReadingNexus2Entry& entry);
    QList<ReadingNexus2Entry> entries() const;
    int masteredCount() const;
    qreal avgComprehension() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void nexusUpdated(int id, qreal comprehension);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawNexusView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingNexus2Entry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
