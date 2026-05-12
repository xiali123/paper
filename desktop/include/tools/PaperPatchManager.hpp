#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct PatchEntry {
    int id; QString name; QString category; QString severity;
    int files; qreal risk; QString date; bool applied; QColor color;
};
class PaperPatchManager : public QWidget {
    Q_OBJECT
public:
    explicit PaperPatchManager(QWidget* parent = nullptr);
    void addEntry(const PatchEntry& entry);
    QList<PatchEntry> entries() const;
    int appliedCount() const;
    qreal avgRisk() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void patchApplied(int id, qreal risk);
private slots:
    void onApply();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawPatchList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<PatchEntry> entries_;
    QSettings settings_;
    QPushButton* applyBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
