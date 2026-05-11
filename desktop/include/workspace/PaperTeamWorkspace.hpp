#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct TeamEntry {
    int id;
    QString member;
    QString role;
    QString category;
    QString status;
    int tasks;
    qreal contribution;
    QString joined;
    bool active;
    QColor color;
};

class PaperTeamWorkspace : public QWidget {
    Q_OBJECT
public:
    explicit PaperTeamWorkspace(QWidget* parent = nullptr);
    void addEntry(const TeamEntry& entry);
    QList<TeamEntry> entries() const;
    int activeCount() const;
    qreal avgContribution() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void memberAdded(int id, qreal contribution);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawTeamList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<TeamEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
