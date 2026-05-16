#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ConfigValEntry {
    int id; QString file; QString category; QString rule;
    qreal coverage; int checks; bool passed; QColor color;
};
class PaperConfigValidator : public QWidget {
    Q_OBJECT
public:
    explicit PaperConfigValidator(QWidget* parent = nullptr);
    void addEntry(const ConfigValEntry& entry);
    QList<ConfigValEntry> entries() const;
    int passedCount() const;
    qreal avgCoverage() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void configValidated(int id, qreal coverage);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawValidationView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConfigValEntry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
