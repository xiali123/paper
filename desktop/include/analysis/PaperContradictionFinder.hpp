#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct ContradictionEntry {
    int id;
    QString claim1;
    QString claim2;
    QString category;
    QString severity;
    qreal conflict;
    int section1;
    int section2;
    bool resolved;
    QColor color;
};

class PaperContradictionFinder : public QWidget {
    Q_OBJECT
public:
    explicit PaperContradictionFinder(QWidget* parent = nullptr);
    void addEntry(const ContradictionEntry& entry);
    QList<ContradictionEntry> entries() const;
    int resolvedCount() const;
    qreal avgConflict() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void contradictionFound(int id, qreal conflict);

private slots:
    void onFind();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawContradictionList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<ContradictionEntry> entries_;
    QSettings settings_;
    QPushButton* findBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
