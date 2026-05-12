#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StudyGroupEntry {
    int id; QString name; QString category; QString role;
    qreal activity; int members; bool active; QColor color;
};
class PaperStudyGroup : public QWidget {
    Q_OBJECT
public:
    explicit PaperStudyGroup(QWidget* parent = nullptr);
    void addEntry(const StudyGroupEntry& entry);
    QList<StudyGroupEntry> entries() const;
    int activeCount() const;
    qreal avgActivity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void groupJoined(int id, qreal activity);
private slots:
    void onJoin();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGroupList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StudyGroupEntry> entries_;
    QSettings settings_;
    QPushButton* joinBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
