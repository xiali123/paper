#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QVector>

struct WorkshopEntry {
    int id;
    QString topic;
    QString facilitator;
    QString date;
    int participants;
    qreal rating;
    QString category;
    int duration;
    bool completed;
    QColor color;
};

class PaperReadingWorkshop : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingWorkshop(QWidget* parent = nullptr);
    void addEntry(const WorkshopEntry& entry);
    QList<WorkshopEntry> entries() const;
    int completedCount() const;
    qreal avgRating() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void workshopCreated(int id, qreal rating);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawWorkshopList(QPainter& p, const QRect& rect);
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
    QList<WorkshopEntry> entries_;
};
