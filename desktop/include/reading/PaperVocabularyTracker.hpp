#pragma once
#include <QWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSettings>
#include <QPainter>
#include <QList>

struct VocabEntry {
    int id;
    QString term;
    QString definition;
    QString context;
    QString category;
    int encounterCount;
    qreal mastery;
    QString source;
    bool mastered;
    QColor color;
};

class PaperVocabularyTracker : public QWidget {
    Q_OBJECT
public:
    explicit PaperVocabularyTracker(QWidget* parent = nullptr);
    void addEntry(const VocabEntry& entry);
    QList<VocabEntry> entries() const;
    int masteredCount() const;
    qreal avgMastery() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void termAdded(int id, const QString& term);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawTermList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QLineEdit* inputField_;
    QComboBox* categoryCombo_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QLabel* infoLabel_;
    QList<VocabEntry> entries_;
    QSettings settings_;
};
