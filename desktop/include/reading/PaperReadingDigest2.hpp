#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingDigest2Entry {
    int id; QString paper; QString category; QString summary;
    qreal comprehension; int pages; bool completed; QColor color;
};
class PaperReadingDigest2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingDigest2(QWidget* parent = nullptr);
    void addEntry(const ReadingDigest2Entry& entry);
    QList<ReadingDigest2Entry> entries() const;
    int completedCount() const;
    qreal avgComprehension() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void digestCreated(int id, qreal comprehension);
private slots:
    void onDigest();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDigestView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingDigest2Entry> entries_;
    QSettings settings_;
    QPushButton* digestBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
