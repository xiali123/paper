#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct ReadingCluster {
    int id;
    QString clusterName;
    QString category;
    int paperCount;
    qreal avgDifficulty;
    qreal completionRate;
    QString priority;
    int members;
    bool active;
    QColor color;
};

class PaperReadingCluster : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingCluster(QWidget* parent = nullptr);
    void addEntry(const ReadingCluster& entry);
    QList<ReadingCluster> entries() const;
    int activeCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void clusterCreated(int id, int paperCount);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawClusterGrid(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLabel* infoLabel_;
    QSettings settings_;
    QList<ReadingCluster> entries_;
};
