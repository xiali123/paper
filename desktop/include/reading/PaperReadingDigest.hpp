#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct DigestEntry {
    int id; QString paper; QString category; QString summary;
    qreal relevance; qreal quality; QString date; bool starred; QColor color;
};
class PaperReadingDigest : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingDigest(QWidget* parent = nullptr);
    void addEntry(const DigestEntry& entry);
    QList<DigestEntry> entries() const;
    int starredCount() const;
    qreal avgQuality() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void digestCreated(int id, qreal quality);
private slots:
    void onCreate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawDigestList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<DigestEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
