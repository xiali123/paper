#pragma once
#include <QWidget>
#include <QSettings>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>

struct FeedbackEntry {
    int id;
    QString paperTitle;
    QString recommendation;
    QString feedback;
    qreal rating;
    QString source;
    bool accepted;
    QString reason;
    QColor color;
};

class PaperRecommenderFeedback : public QWidget {
    Q_OBJECT
public:
    explicit PaperRecommenderFeedback(QWidget* parent = nullptr);
    void addFeedback(const FeedbackEntry& entry);
    QList<FeedbackEntry> entries() const;
    int acceptedCount() const;
    qreal avgRating() const;
    QMap<QString, int> sourceCounts() const;

signals:
    void feedbackRecorded(int id, bool accepted);

protected:
    void paintEvent(QPaintEvent*) override;

private:
    void setupUI();
    void onAdd();
    void onClear();
    void drawFeedbackList(QPainter& p, const QRect& rect);
    void drawAcceptChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void loadSettings();
    void saveSettings();

    QSettings settings_;
    QList<FeedbackEntry> entries_;
    QPushButton* addBtn_;
    QPushButton* clearBtn_;
    QLineEdit* inputField_;
    QComboBox* filterCombo_;
    QLabel* infoLabel_;
};
