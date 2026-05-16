#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct StrengthAssessor2Entry {
    int id; QString claim; QString category; QString evidence;
    qreal strength; int sources; bool robust; QColor color;
};
class PaperStrengthAssessor2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperStrengthAssessor2(QWidget* parent = nullptr);
    void addEntry(const StrengthAssessor2Entry& entry);
    QList<StrengthAssessor2Entry> entries() const;
    int robustCount() const;
    qreal avgStrength() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void claimAssessed(int id, qreal strength);
private slots:
    void onAssess();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawAssessorView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<StrengthAssessor2Entry> entries_;
    QSettings settings_;
    QPushButton* assessBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
