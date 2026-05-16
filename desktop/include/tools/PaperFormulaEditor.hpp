#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct FormulaEntry {
    int id; QString name; QString category; QString expression;
    qreal result; qreal accuracy; int variables; bool verified; QColor color;
};
class PaperFormulaEditor : public QWidget {
    Q_OBJECT
public:
    explicit PaperFormulaEditor(QWidget* parent = nullptr);
    void addEntry(const FormulaEntry& entry);
    QList<FormulaEntry> entries() const;
    int verifiedCount() const;
    qreal avgAccuracy() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void formulaSaved(int id, qreal accuracy);
private slots:
    void onSave();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawFormulaList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<FormulaEntry> entries_;
    QSettings settings_;
    QPushButton* saveBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
