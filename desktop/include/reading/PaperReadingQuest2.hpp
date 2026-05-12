#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ReadingQuest2Entry {
    int id; QString quest; QString category; QString difficulty;
    qreal completion; int rewards; bool legendary; QColor color;
};
class PaperReadingQuest2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperReadingQuest2(QWidget* parent = nullptr);
    void addEntry(const ReadingQuest2Entry& entry);
    QList<ReadingQuest2Entry> entries() const;
    int legendaryCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void questComplete(int id, qreal completion);
private slots:
    void onAccept();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawQuestView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ReadingQuest2Entry> entries_;
    QSettings settings_;
    QPushButton* acceptBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
