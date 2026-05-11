#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct BoardEntry {
    int id; QString project; QString category; QString status;
    int tasks; qreal completion; QString deadline; bool onTrack; QColor color;
};
class PaperProjectBoard : public QWidget {
    Q_OBJECT
public:
    explicit PaperProjectBoard(QWidget* parent = nullptr);
    void addEntry(const BoardEntry& entry);
    QList<BoardEntry> entries() const;
    int onTrackCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void boardUpdated(int id, qreal completion);
private slots:
    void onUpdate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawBoardList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<BoardEntry> entries_;
    QSettings settings_;
    QPushButton* updateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
