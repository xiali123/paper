#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LogicValidatorEntry {
    int id; QString premise; QString category; QString conclusion;
    qreal validity; int steps; bool sound; QColor color;
};
class PaperLogicValidator : public QWidget {
    Q_OBJECT
public:
    explicit PaperLogicValidator(QWidget* parent = nullptr);
    void addEntry(const LogicValidatorEntry& entry);
    QList<LogicValidatorEntry> entries() const;
    int soundCount() const;
    qreal avgValidity() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void logicValidated(int id, qreal validity);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawLogicView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LogicValidatorEntry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
