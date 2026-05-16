#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct SurveyEntry {
    int id; QString survey; QString category; QString status;
    int responses; qreal completion; QString deadline; bool active; QColor color;
};
class PaperSurveyManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperSurveyManager(QWidget* parent = nullptr);
    void addEntry(const SurveyEntry& entry);
    QList<SurveyEntry> entries() const;
    int activeCount() const;
    qreal avgCompletion() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void surveyCreated(int id, qreal completion);
private slots:
    void onCreate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawSurveyList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<SurveyEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
