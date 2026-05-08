#pragma once

#include <QWidget>
#include <QPainter>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>
#include <QSettings>

struct DiffLine {
    int lineNumber{0};
    QString text;
    QString type; // "added", "removed", "unchanged"
    QColor color;
};

struct DiffResult {
    int added{0};
    int removed{0};
    int unchanged{0};
    QList<DiffLine> lines;
};

class PaperVersionDiff : public QWidget {
    Q_OBJECT

public:
    explicit PaperVersionDiff(QWidget* parent = nullptr);

    void setVersions(const QString& oldText, const QString& newText);
    DiffResult diff() const;
    qreal similarity() const;
    int addedLines() const;
    int removedLines() const;

signals:
    void diffComputed(int added, int removed);

private slots:
    void onLoad();
    void onClear();

private:
    void setupUI();
    void paintEvent(QPaintEvent* event) override;
    void drawDiffView(QPainter& p, const QRect& rect);
    void drawStats(QPainter& p, const QRect& rect);
    void drawMiniMap(QPainter& p, const QRect& rect);
    void updateInfo();
    void computeDiff();

    QPushButton* loadBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QLabel* infoLabel_{nullptr};

    QString oldText_;
    QString newText_;
    DiffResult diffResult_;
    QSettings settings_;
};
