#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct CitationSankeyEntry {
    int id;
    QString source;
    QString target;
    qreal weight;
    QString flowType;
    int year;
    QString field;
    qreal impact;
    QString label;
    QColor color;
};

class PaperCitationSankey : public QWidget {
    Q_OBJECT
public:
    explicit PaperCitationSankey(QWidget* parent = nullptr);
    void addEntry(const CitationSankeyEntry& entry);
    QList<CitationSankeyEntry> entries() const;
    qreal totalWeight() const;
    qreal avgImpact() const;
    QMap<QString, int> fieldCounts() const;

signals:
    void sankeyGenerated(int id, qreal weight);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onGenerate();
    void onClear();
    void drawSankeyView(QPainter& p, const QRect& rect);
    void drawFieldChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* fieldCombo_;
    QPushButton* generateBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<CitationSankeyEntry> entries_;
    QSettings settings_;
};
