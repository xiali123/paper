#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct MatchEntry {
    int id;
    QString researcherA;
    QString researcherB;
    qreal compatibility;
    QStringList sharedTopics;
    QString collaborationType;
    int sharedPapers;
    qreal score;
    QColor color;
};

class PaperCollaborationMatcher : public QWidget {
    Q_OBJECT
public:
    explicit PaperCollaborationMatcher(QWidget* parent = nullptr);
    void addMatch(const MatchEntry& entry);
    QList<MatchEntry> matches() const;
    QMap<QString, int> typeCounts() const;
    qreal avgCompatibility() const;
    int totalSharedPapers() const;

signals:
    void matchFound(int id, qreal compatibility);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onMatch();
    void onClear();
    void drawMatchList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<MatchEntry> matches_;
    QPushButton* matchBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
