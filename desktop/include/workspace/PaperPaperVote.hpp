#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct VoteEntry {
    int id; QString paper; QString category; QString stance;
    qreal score; int voters; bool consensus; QColor color;
};
class PaperPaperVote : public QWidget {
    Q_OBJECT
public:
    explicit PaperPaperVote(QWidget* parent = nullptr);
    void addEntry(const VoteEntry& entry);
    QList<VoteEntry> entries() const;
    int consensusCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void voteCast(int id, qreal score);
private slots:
    void onVote();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawVoteBoard(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<VoteEntry> entries_;
    QSettings settings_;
    QPushButton* voteBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
