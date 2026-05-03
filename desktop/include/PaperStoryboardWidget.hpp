#pragma once

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QList>
#include <QMap>

struct StoryCard {
    int id{-1};
    int paperId{-1};
    QString title;
    QString note;
    QPointF pos;
    QColor color{QColor(59, 130, 246)};
    int slotIndex{-1};
};

class PaperStoryboardWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperStoryboardWidget(QWidget* parent = nullptr);

    void addPaper(int paperId, const QString& title);
    void addPapers(const QList<QPair<int, QString>>& papers);
    void clearBoard();
    QList<StoryCard> cards() const;
    void setSlotCount(int count);
    int slotCount() const;

signals:
    void cardMoved(int cardId, int slotIndex);
    void cardClicked(int paperId);
    void boardChanged();
    void exportRequested();

private slots:
    void onAddCard();
    void onRemoveCard();
    void onClear();
    void onAutoArrange();
    void onExport();
    void onSlotCountChanged(int val);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private:
    void setupUI();
    int hitTestCard(const QPointF& pos);
    int hitTestSlot(const QPointF& pos);

    QListWidget* cardPool_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* addBtn_{nullptr};
    QPushButton* removeBtn_{nullptr};
    QPushButton* clearBtn_{nullptr};
    QPushButton* autoBtn_{nullptr};
    QPushButton* exportBtn_{nullptr};

    QList<StoryCard> cards_;
    int nextCardId_{1};
    int slots_{5};
    int dragCard_{-1};
    QPointF dragOffset_;
    QPointF dragStartPos_;
    bool dragging_{false};

    static constexpr qreal cardW_{160};
    static constexpr qreal cardH_{80};
    static constexpr qreal slotW_{180};
    static constexpr qreal slotH_{100};
    static constexpr qreal slotSpacing_{20};
    static constexpr qreal slotY_{20};
};
