#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct TagInfo {
    QString name;
    int count{0};
    QColor color;
    qreal weight{0};
};

class PaperTagCloudWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperTagCloudWidget(QWidget* parent = nullptr);

    void setTags(const QMap<QString, int>& tags);
    void addTag(const QString& name, int count = 1);
    QList<TagInfo> tags() const;
    QStringList topTags(int limit = 10) const;
    int totalTagCount() const;
    int uniqueTagCount() const;

signals:
    void tagClicked(const QString& tag);
    void tagsChanged(int unique, int total);

private slots:
    void onLayoutChanged(int index);
    void onSortChanged(int index);
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawCloud(QPainter& p, const QRect& rect);
    void drawBarChart(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void updateInfo();
    void rebuildTags();

    QComboBox* layoutCombo_{nullptr};
    QComboBox* sortCombo_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QList<TagInfo> tags_;
    QSettings settings_;
};
