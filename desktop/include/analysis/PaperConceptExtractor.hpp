#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QVector>

struct ConceptEntry {
    int id;
    QString concept;
    qreal relevance;
    QString domain;
    int coOccurrences;
    QString definition;
    qreal frequency;
    QString category;
    bool keyConcept;
    QColor color;
};

class PaperConceptExtractor : public QWidget {
    Q_OBJECT
public:
    explicit PaperConceptExtractor(QWidget* parent = nullptr);
    void addEntry(const ConceptEntry& entry);
    QList<ConceptEntry> entries() const;
    qreal avgRelevance() const;
    int keyConceptCount() const;
    QMap<QString, int> domainCounts() const;

signals:
    void conceptExtracted(int id, qreal relevance);

private slots:
    void onExtract();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawConceptList(QPainter& p, const QRect& rect);
    void drawDomainChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QVector<ConceptEntry> entries_;
    QPushButton* extractBtn_;
    QPushButton* clearBtn_;
    QComboBox* domainCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
