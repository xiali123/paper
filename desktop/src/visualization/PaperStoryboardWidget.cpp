#include "visualization/PaperStoryboardWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QInputDialog>
#include <QSpinBox>

PaperStoryboardWidget::PaperStoryboardWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void PaperStoryboardWidget::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);
    auto* splitter = new QSplitter(Qt::Horizontal);

    // Left: card pool
    auto* leftPanel = new QVBoxLayout();

    auto* slotRow = new QHBoxLayout();
    slotRow->addWidget(new QLabel("Slots:"));
    auto* slotSpin = new QSpinBox();
    slotSpin->setRange(2, 12);
    slotSpin->setValue(slots_);
    connect(slotSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, &PaperStoryboardWidget::onSlotCountChanged);
    slotRow->addWidget(slotSpin);
    leftPanel->addLayout(slotRow);

    cardPool_ = new QListWidget();
    cardPool_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    leftPanel->addWidget(cardPool_, 1);

    auto* btnRow = new QHBoxLayout();
    addBtn_ = new QPushButton("Add");
    addBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 4px 10px; border-radius: 4px; }");
    connect(addBtn_, &QPushButton::clicked, this, &PaperStoryboardWidget::onAddCard);
    btnRow->addWidget(addBtn_);

    removeBtn_ = new QPushButton("Remove");
    removeBtn_->setStyleSheet("color: #dc2626;");
    connect(removeBtn_, &QPushButton::clicked, this, &PaperStoryboardWidget::onRemoveCard);
    btnRow->addWidget(removeBtn_);

    leftPanel->addLayout(btnRow);

    auto* btnRow2 = new QHBoxLayout();
    autoBtn_ = new QPushButton("Auto Arrange");
    connect(autoBtn_, &QPushButton::clicked, this, &PaperStoryboardWidget::onAutoArrange);
    btnRow2->addWidget(autoBtn_);

    clearBtn_ = new QPushButton("Clear");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperStoryboardWidget::onClear);
    btnRow2->addWidget(clearBtn_);

    exportBtn_ = new QPushButton("Export");
    connect(exportBtn_, &QPushButton::clicked, this, &PaperStoryboardWidget::onExport);
    btnRow2->addWidget(exportBtn_);

    leftPanel->addLayout(btnRow2);

    auto* leftWidget = new QWidget();
    leftWidget->setLayout(leftPanel);
    splitter->addWidget(leftWidget);

    // Right: storyboard canvas
    auto* rightPanel = new QVBoxLayout();
    auto* canvasLabel = new QLabel("Story Board — drag cards to slots");
    canvasLabel->setStyleSheet("font-weight: bold; font-size: 12px;");
    rightPanel->addWidget(canvasLabel);

    // Canvas is this widget itself (we paint on it)
    auto* canvasWidget = new QWidget();
    canvasWidget->setMinimumHeight(200);
    rightPanel->addWidget(canvasWidget, 1);

    infoLabel_ = new QLabel("Drag cards from pool to storyboard slots");
    infoLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    rightPanel->addWidget(infoLabel_);

    auto* rightWidget = new QWidget();
    rightWidget->setLayout(rightPanel);
    splitter->addWidget(rightWidget);

    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    mainLayout->addWidget(splitter);

    setMinimumHeight(300);
}

void PaperStoryboardWidget::addPaper(int paperId, const QString& title) {
    StoryCard card;
    card.id = nextCardId_++;
    card.paperId = paperId;
    card.title = title.length() > 30 ? title.left(27) + "..." : title;
    card.slotIndex = -1;
    cards_.append(card);
    cardPool_->addItem(QString("[%1] %2").arg(card.id).arg(card.title));
    infoLabel_->setText(QString("%1 cards, %2 on board").arg(cards_.size()).arg(
        std::count_if(cards_.begin(), cards_.end(), [](const StoryCard& c) { return c.slotIndex >= 0; })));
}

void PaperStoryboardWidget::addPapers(const QList<QPair<int, QString>>& papers) {
    for (const auto& [pid, title] : papers) addPaper(pid, title);
}

void PaperStoryboardWidget::clearBoard() {
    cards_.clear();
    cardPool_->clear();
    nextCardId_ = 1;
    update();
    infoLabel_->setText("Board cleared");
    emit boardChanged();
}

QList<StoryCard> PaperStoryboardWidget::cards() const { return cards_; }

void PaperStoryboardWidget::setSlotCount(int count) {
    slots_ = qBound(2, count, 12);
    update();
}

int PaperStoryboardWidget::slotCount() const { return slots_; }

void PaperStoryboardWidget::onAddCard() {
    QString title = QInputDialog::getText(this, "Add Card", "Card title:");
    if (title.trimmed().isEmpty()) return;
    addPaper(0, title);
}

void PaperStoryboardWidget::onRemoveCard() {
    int row = cardPool_->currentRow();
    if (row < 0 || row >= cards_.size()) return;
    cards_.removeAt(row);
    cardPool_->takeItem(row);
    update();
    emit boardChanged();
}

void PaperStoryboardWidget::onClear() { clearBoard(); }

void PaperStoryboardWidget::onAutoArrange() {
    int col = 0;
    for (auto& card : cards_) {
        card.slotIndex = col % slots_;
        qreal x = 20 + (card.slotIndex % 3) * (slotW_ + slotSpacing_);
        qreal y = slotY_ + (card.slotIndex / 3) * (slotH_ + slotSpacing_);
        card.pos = QPointF(x, y);
        col++;
    }
    update();
    infoLabel_->setText(QString("Auto-arranged %1 cards into %2 slots").arg(cards_.size()).arg(slots_));
    emit boardChanged();
}

void PaperStoryboardWidget::onExport() { emit exportRequested(); }

void PaperStoryboardWidget::onSlotCountChanged(int val) {
    setSlotCount(val);
    update();
}

void PaperStoryboardWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QRect canvasRect = rect().adjusted(200, 0, 0, -40);
    if (canvasRect.width() < 100 || canvasRect.height() < 100) return;

    // Draw slots
    for (int i = 0; i < slots_; ++i) {
        qreal x = canvasRect.x() + 10 + (i % 3) * (slotW_ + slotSpacing_);
        qreal y = canvasRect.y() + slotY_ + (i / 3) * (slotH_ + slotSpacing_);

        p.setPen(QPen(QColor(203, 213, 225), 2, Qt::DashLine));
        p.setBrush(QColor(248, 250, 252));
        p.drawRoundedRect(QRectF(x, y, slotW_, slotH_), 8, 8);

        p.setPen(QColor(148, 163, 184));
        QFont font = p.font();
        font.setPixelSize(10);
        p.setFont(font);
        p.drawText(QRectF(x, y, slotW_, 16), Qt::AlignCenter, QString("Slot %1").arg(i + 1));
    }

    // Draw connection arrows between slotted cards
    QPen arrowPen(QColor(148, 163, 184), 2);
    arrowPen.setStyle(Qt::SolidLine);
    p.setPen(arrowPen);
    StoryCard* prev = nullptr;
    for (int s = 0; s < slots_; ++s) {
        StoryCard* cur = nullptr;
        for (auto& card : cards_) {
            if (card.slotIndex == s) { cur = &card; break; }
        }
        if (prev && cur) {
            QPointF from(prev->pos.x() + cardW_ / 2, prev->pos.y() + cardH_);
            QPointF to(cur->pos.x() + cardW_ / 2, cur->pos.y());
            p.drawLine(from, to);
            // Arrowhead
            qreal angle = atan2(to.y() - from.y(), to.x() - from.x());
            qreal sz = 8;
            QPointF p1(to.x() - sz * cos(angle - 0.4), to.y() - sz * sin(angle - 0.4));
            QPointF p2(to.x() - sz * cos(angle + 0.4), to.y() - sz * sin(angle + 0.4));
            p.drawLine(to, p1);
            p.drawLine(to, p2);
        }
        if (cur) prev = cur;
    }

    // Draw cards
    for (const auto& card : cards_) {
        if (card.slotIndex < 0) continue;
        QRectF cr(card.pos.x(), card.pos.y(), cardW_, cardH_);

        // Shadow
        p.setPen(Qt::NoPen);
        p.setBrush(QColor(0, 0, 0, 20));
        p.drawRoundedRect(cr.translated(2, 2), 6, 6);

        // Card body
        p.setBrush(card.color.lighter(160));
        p.setPen(QPen(card.color, 1));
        p.drawRoundedRect(cr, 6, 6);

        // Header bar
        p.setBrush(card.color);
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(QRectF(cr.x(), cr.y(), cr.width(), 20), 6, 6);
        p.drawRect(QRectF(cr.x(), cr.y() + 10, cr.width(), 10));

        // Slot number
        p.setPen(Qt::white);
        QFont font = p.font();
        font.setPixelSize(9);
        font.setBold(true);
        p.setFont(font);
        p.drawText(QRectF(cr.x(), cr.y(), cr.width(), 20), Qt::AlignCenter,
                   QString("#%1").arg(card.slotIndex + 1));

        // Title
        p.setPen(QColor(30, 41, 59));
        font.setPixelSize(11);
        font.setBold(false);
        p.setFont(font);
        p.drawText(cr.adjusted(6, 22, -6, -4), Qt::AlignLeft | Qt::TextWordWrap, card.title);
    }
}

void PaperStoryboardWidget::mousePressEvent(QMouseEvent* event) {
    QPointF pos = event->pos();
    dragCard_ = hitTestCard(pos);
    if (dragCard_ >= 0) {
        dragStartPos_ = cards_[dragCard_].pos;
        dragOffset_ = pos - cards_[dragCard_].pos;
        dragging_ = true;
    }
}

void PaperStoryboardWidget::mouseMoveEvent(QMouseEvent* event) {
    if (!dragging_ || dragCard_ < 0) return;
    cards_[dragCard_].pos = event->pos() - dragOffset_;
    update();
}

void PaperStoryboardWidget::mouseReleaseEvent(QMouseEvent* event) {
    if (dragging_ && dragCard_ >= 0) {
        QPointF pos = event->pos();
        int slot = hitTestSlot(pos);
        if (slot >= 0) {
            cards_[dragCard_].slotIndex = slot;
            emit cardMoved(cards_[dragCard_].id, slot);
        }
        emit boardChanged();
    }
    dragging_ = false;
    dragCard_ = -1;
    update();
    int onBoard = std::count_if(cards_.begin(), cards_.end(), [](const StoryCard& c) { return c.slotIndex >= 0; });
    infoLabel_->setText(QString("%1 cards, %2 on board").arg(cards_.size()).arg(onBoard));
}

int PaperStoryboardWidget::hitTestCard(const QPointF& pos) {
    for (int i = 0; i < cards_.size(); ++i) {
        QRectF cr(cards_[i].pos.x(), cards_[i].pos.y(), cardW_, cardH_);
        if (cr.contains(pos)) return i;
    }
    return -1;
}

int PaperStoryboardWidget::hitTestSlot(const QPointF& pos) {
    QRect canvasRect = rect().adjusted(200, 0, 0, -40);
    for (int i = 0; i < slots_; ++i) {
        qreal x = canvasRect.x() + 10 + (i % 3) * (slotW_ + slotSpacing_);
        qreal y = canvasRect.y() + slotY_ + (i / 3) * (slotH_ + slotSpacing_);
        QRectF sr(x, y, slotW_, slotH_);
        if (sr.contains(pos)) return i;
    }
    return -1;
}
