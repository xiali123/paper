#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct ContradictionEntry {
    int id;
    QString claimA;
    QString claimB;
    QString conflictType;
    qreal severity;
    QString evidence;
    QString resolution;
    QString source;
    bool resolved;
    QColor color;
};

class PaperContradictionDetector : public QWidget {
    Q_OBJECT
public:
    explicit PaperContradictionDetector(QWidget* parent = nullptr);
    void addEntry(const ContradictionEntry& entry);
    QList<ContradictionEntry> entries() const;
    qreal avgSeverity() const;
    int unresolvedCount() const;
    QMap<QString, int> typeCounts() const;

signals:
    void contradictionFound(int id, qreal severity);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onDetect();
    void onClear();
    void drawConflictList(QPainter& p, const QRect& rect);
    void drawTypeChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QPushButton* detectBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<ContradictionEntry> entries_;
    QSettings settings_;
};
