#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QList>
#include <QMap>
#include <QSettings>

struct TopicWord {
    QString word;
    qreal weight{0};
};

struct Topic {
    int id{-1};
    QString label;
    QList<TopicWord> words;
    qreal proportion{0};
    QColor color;
};

class PaperTopicModeler : public QWidget {
    Q_OBJECT

public:
    explicit PaperTopicModeler(QWidget* parent = nullptr);

    void setPapers(const QList<QPair<int, QString>>& papers);
    void model(int numTopics = 5);
    QList<Topic> topics() const;
    void clear();

signals:
    void modelingComplete(int topicCount);
    void topicClicked(int topicId, const QString& label);

private slots:
    void onModel();
    void onTopicCountChanged(int index);
    void onTopicClicked();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawTopicBars(QPainter& p, const QRect& rect);
    void drawWordCloud(QPainter& p, const QRect& rect);
    void refreshList();
    void updateStats();

    QComboBox* topicCountCombo_{nullptr};
    QPushButton* modelBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QListWidget* topicList_{nullptr};
    QLabel* statsLabel_{nullptr};

    QList<Topic> topics_;
    int selectedTopic_{-1};
};
