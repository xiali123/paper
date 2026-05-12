#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SkillEntry {
    int id; QString skill; QString category; QString level;
    qreal proficiency; int projects; bool expert; QColor color;
};
class PaperSkillMatrix : public QWidget {
    Q_OBJECT
public:
    explicit PaperSkillMatrix(QWidget* parent = nullptr);
    void addEntry(const SkillEntry& entry);
    QList<SkillEntry> entries() const;
    int expertCount() const;
    qreal avgProficiency() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void skillAssessed(int id, qreal proficiency);
private slots:
    void onAssess();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawMatrixView(QPainter& p, const QRect& rect);
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
