#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct EvidenceGraderEntry {
    int id; QString claim; QString category; QString source;
    qreal grade; int citations; bool strong; QColor color;
};
class PaperEvidenceGrader : public QWidget {
    Q_OBJECT
public:
    explicit PaperEvidenceGrader(QWidget* parent = nullptr);
    void addEntry(const EvidenceGraderEntry& entry);
    QList<EvidenceGraderEntry> entries() const;
    int strongCount() const;
    qreal avgGrade() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void evidenceGraded(int id, qreal grade);
private slots:
    void onGrade();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawGradeView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<EvidenceGraderEntry> entries_;
    QSettings settings_;
    QPushButton* gradeBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
