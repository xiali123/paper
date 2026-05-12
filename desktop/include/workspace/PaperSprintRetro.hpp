#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SprintRetroEntry {
    int id; QString sprint; QString category; QString feedback;
    qreal score; int actionItems; bool resolved; QColor color;
};
class PaperSprintRetro : public QWidget {
    Q_OBJECT
public:
    explicit PaperSprintRetro(QWidget* parent = nullptr);
    void addEntry(const SprintRetroEntry& entry);
    QList<SprintRetroEntry> entries() const;
    int resolvedCount() const;
    qreal avgScore() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void retroCompleted(int id, qreal score);
private slots:
    void onReview();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawRetroView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SprintRetroEntry> entries_;
    QSettings settings_;
    QPushButton* reviewBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
