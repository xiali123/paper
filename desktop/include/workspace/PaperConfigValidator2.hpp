#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct ConfigValidator2Entry {
    int id; QString config; QString category; QString rule;
    qreal compliance; int checks; bool passed; QColor color;
};
class PaperConfigValidator2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperConfigValidator2(QWidget* parent = nullptr);
    void addEntry(const ConfigValidator2Entry& entry);
    QList<ConfigValidator2Entry> entries() const;
    int passedCount() const;
    qreal avgCompliance() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void validationComplete(int id, qreal compliance);
private slots:
    void onValidate();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawValidatorView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<ConfigValidator2Entry> entries_;
    QSettings settings_;
    QPushButton* validateBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
