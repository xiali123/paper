#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct ConferenceEntry {
    int id;
    QString name;
    QString category;
    QString deadline;
    QString date;
    QString location;
    qreal fee;
    bool submitted;
    QColor color;
};

class PaperConferenceTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperConferenceTracker(QWidget* parent = nullptr);
    void addEntry(const ConferenceEntry& entry);
    QList<ConferenceEntry> entries() const;
    int submittedCount() const;
    qreal totalFee() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void conferenceAdded(int id, qreal fee);
private slots:
    void onAdd();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawConferenceList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConferenceEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
