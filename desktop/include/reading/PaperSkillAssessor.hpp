#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct SkillEntry {
    int id;
    QString skill;
    QString category;
    QString level;
    qreal progress;
    int experience;
    bool certified;
    QColor color;
};

class PaperSkillAssessor : public QWidget {
    Q_OBJECT
public:
    explicit PaperSkillAssessor(QWidget* parent = nullptr);
    void addEntry(const SkillEntry& entry);
    QList<SkillEntry> entries() const;
    int certifiedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void skillAssessed(int id, qreal progress);

private slots:
    void onAssess();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawSkillList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<SkillEntry> entries_;
    QSettings settings_;
    QPushButton* assessBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
