#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct ProgressGridEntry {
    int id;
    QString userName;
    qreal progress;
    QString category;
    int papersRead;
    int papersTotal;
    QString week;
    qreal speed;
    bool onTrack;
    QColor color;
};

class PaperReadingProgressGrid : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingProgressGrid(QWidget* parent = nullptr);
    void addEntry(const ProgressGridEntry& entry);
    QList<ProgressGridEntry> entries() const;
    qreal avgProgress() const;
    int onTrackCount() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void progressUpdated(int id, qreal progress);

private slots:
    void onUpdate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawProgressGrid(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<ProgressGridEntry> entries_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
