#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SkillMatrix2Entry {
    int id; QString skill; QString category; QString level;
    qreal proficiency; int projects; bool expert; QColor color;
};
class PaperSkillMatrix2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperSkillMatrix2(QWidget* parent = nullptr);
    void addEntry(const SkillMatrix2Entry& entry);
    QList<SkillMatrix2Entry> entries() const;
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
    QList<SkillMatrix2Entry> entries_;
    QSettings settings_;
    QPushButton* assessBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
