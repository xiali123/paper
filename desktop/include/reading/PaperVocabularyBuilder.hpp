#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct VocabEntry {
    int id;
    QString word;
    QString definition;
    QString category;
    QString level;
    int occurrences;
    qreal mastery;
    bool learned;
    QColor color;
};

class PaperVocabularyBuilder : public QWidget {
    Q_OBJECT
public:
    explicit PaperVocabularyBuilder(QWidget* parent = nullptr);
    void addEntry(const VocabEntry& entry);
    QList<VocabEntry> entries() const;
    int learnedCount() const;
    qreal avgMastery() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void wordAdded(int id, qreal mastery);

private slots:
    void onAdd();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawVocabList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<VocabEntry> entries_;
    QSettings settings_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
