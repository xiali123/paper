#pragma once
#include <QWidget>
#include <QSettings>
#include <QPainter>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

struct FlashcardEntry {
    int id;
    QString front;
    QString back;
    QString category;
    QString difficulty;
    int interval;
    int reviews;
    qreal mastery;
    bool mastered;
    QColor color;
};

class PaperFlashcardDeck : public QWidget {
    Q_OBJECT
public:
    explicit PaperFlashcardDeck(QWidget* parent = nullptr);
    void addEntry(const FlashcardEntry& entry);
    QList<FlashcardEntry> entries() const;
    int masteredCount() const;
    qreal avgMastery() const;
    QMap<QString, int> categoryCounts() const;

signals:
    void cardCreated(int id, qreal mastery);

private slots:
    void onCreate();
    void onClear();

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void drawCardList(QPainter& p, const QRect& rect);
    void drawCategoryChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QList<FlashcardEntry> entries_;
    QSettings settings_;
    QPushButton* createBtn_;
    QPushButton* clearBtn_;
    QComboBox* categoryCombo_;
    QLineEdit* inputField_;
    QLabel* infoLabel_;
};
