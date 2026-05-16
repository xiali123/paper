#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
struct LabNotebook2Entry {
    int id; QString experiment; QString category; QString notebook;
    qreal progress; int entries; bool pinned; QColor color;
};
class PaperLabNotebook2 : public QWidget {
    Q_OBJECT
public:
    explicit PaperLabNotebook2(QWidget* parent = nullptr);
    void addEntry(const LabNotebook2Entry& entry);
    QList<LabNotebook2Entry> entries() const;
    int pinnedCount() const;
    qreal avgProgress() const;
    QMap<QString, int> categoryCounts() const;
signals:
    void entryPinned(int id, qreal progress);
private slots:
    void onRecord();
    void onClear();
protected:
    void paintEvent(QPaintEvent*) override;
private:
    void setupUI();
    void drawNotebookView(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();
    QList<LabNotebook2Entry> entries_;
    QSettings settings_;
    QPushButton* recordBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
