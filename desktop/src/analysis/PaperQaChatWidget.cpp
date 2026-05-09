#include "analysis/PaperQaChatWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTime>
#include <QRandomGenerator>

PaperQaChatWidget::PaperQaChatWidget(QWidget* parent)
    : QWidget(parent)
    , settings_("PaperCrawler", "PaperQA")
{
    setupUI();
    loadSettings();
}

void PaperQaChatWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    auto* toolbar = new QHBoxLayout();
    clearBtn_ = new QPushButton("Clear");
    clearBtn_->setStyleSheet("color: #dc2626;");
    connect(clearBtn_, &QPushButton::clicked, this, &PaperQaChatWidget::onClear);
    toolbar->addWidget(clearBtn_);
    toolbar->addStretch();
    layout->addLayout(toolbar);

    infoLabel_ = new QLabel("Ask questions about papers");
    infoLabel_->setStyleSheet("font-size: 12px; color: #334155; padding: 4px;");
    layout->addWidget(infoLabel_);

    auto* inputLayout = new QHBoxLayout();
    inputField_ = new QLineEdit();
    inputField_->setPlaceholderText("Ask a question about the paper...");
    inputField_->setStyleSheet("QLineEdit { padding: 8px; border: 1px solid #cbd5e1; border-radius: 6px; }");
    connect(inputField_, &QLineEdit::returnPressed, this, &PaperQaChatWidget::onSend);
    inputLayout->addWidget(inputField_, 1);

    sendBtn_ = new QPushButton("Send");
    sendBtn_->setStyleSheet("QPushButton { background: #3b82f6; color: white; padding: 8px 16px; border-radius: 6px; }");
    connect(sendBtn_, &QPushButton::clicked, this, &PaperQaChatWidget::onSend);
    inputLayout->addWidget(sendBtn_);
    layout->addLayout(inputLayout);

    setMinimumSize(500, 450);
}

void PaperQaChatWidget::addMessage(const QaMessage& msg) {
    messages_.append(msg);
    saveSettings();
    updateInfo();
    update();
}

QList<QaMessage> PaperQaChatWidget::messages() const { return messages_; }
int PaperQaChatWidget::messageCount() const { return messages_.size(); }

void PaperQaChatWidget::setPaper(const QString& title, int id) {
    paperTitle_ = title;
    paperId_ = id;
    updateInfo();
}

void PaperQaChatWidget::onSend() {
    QString text = inputField_->text().trimmed();
    if (text.isEmpty()) return;
    inputField_->clear();

    QaMessage q;
    q.id = messages_.size() + 1;
    q.text = text;
    q.sender = "user";
    q.time = QTime::currentTime();
    q.paperTitle = paperTitle_;
    q.paperId = paperId_;
    addMessage(q);
    emit questionAsked(text, paperId_);

    // Simulate answer
    QStringList answers = {
        "Based on the paper, the main contribution is a novel architecture that achieves state-of-the-art results.",
        "The methodology uses a two-phase training approach with pre-training followed by fine-tuning.",
        "Key limitations include scalability to larger datasets and generalization across domains.",
        "The authors compare against 5 baselines and report improvements of 2-5% on all metrics.",
        "This paper builds on prior work by Smith et al. (2024) and extends it with attention mechanisms.",
        "The dataset used contains 100K samples across 10 categories with train/test/validation splits.",
        "Future work suggested includes multi-modal extensions and real-time inference optimization.",
        "The proposed method reduces computational cost by 40% while maintaining accuracy."
    };

    QaMessage a;
    a.id = messages_.size() + 1;
    a.text = answers[QRandomGenerator::global()->bounded(answers.size())];
    a.sender = "assistant";
    a.time = QTime::currentTime();
    a.paperTitle = paperTitle_;
    a.paperId = paperId_;
    addMessage(a);
    emit answerReady(a.text);
}

void PaperQaChatWidget::onClear() {
    messages_.clear();
    saveSettings();
    infoLabel_->setText("Ask questions about papers");
    update();
}

void PaperQaChatWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), Qt::white);

    if (messages_.isEmpty()) {
        p.setPen(QColor(203, 213, 225));
        p.setFont(QFont("Arial", 12));
        p.drawText(rect().adjusted(0, 0, 0, -60), Qt::AlignCenter, "Ask questions about papers");
        return;
    }

    drawChat(p, rect().adjusted(20, 10, -20, -55));
}

void PaperQaChatWidget::drawChat(QPainter& p, const QRect& rect) {
    int show = messages_.size();
    int y = rect.bottom();
    qreal bubbleW = rect.width() * 0.7;

    for (int i = show - 1; i >= 0 && y > rect.y(); --i) {
        const auto& m = messages_[i];
        bool isUser = (m.sender == "user");

        int lineCount = qMax(1, static_cast<int>(m.text.length()) / 40) + 1;
        int bubbleH = 16 + lineCount * 16;

        y -= bubbleH + 8;
        if (y < rect.y()) break;

        int bx = isUser ? rect.right() - static_cast<int>(bubbleW) - 10 : rect.x() + 10;

        p.setPen(Qt::NoPen);
        p.setBrush(isUser ? QColor(59, 130, 246) : QColor(241, 245, 249));
        p.drawRoundedRect(bx, y, static_cast<int>(bubbleW), bubbleH, 10, 10);

        p.setPen(isUser ? Qt::white : QColor(15, 23, 42));
        p.setFont(QFont("Arial", 9));
        p.drawText(bx + 10, y + 4, static_cast<int>(bubbleW) - 20, bubbleH - 8,
                   Qt::AlignVCenter | Qt::TextWordWrap, m.text);

        p.setPen(QColor(160, 170, 180));
        p.setFont(QFont("Arial", 7));
        p.drawText(isUser ? bx - 40 : bx + static_cast<int>(bubbleW) + 5, y + bubbleH - 12,
                   40, 12, isUser ? Qt::AlignRight : Qt::AlignLeft, m.time.toString("HH:mm"));
    }
}

void PaperQaChatWidget::updateInfo() {
    if (messages_.isEmpty()) { infoLabel_->setText("Ask questions about papers"); return; }
    int questions = 0;
    for (const auto& m : messages_) if (m.sender == "user") questions++;
    infoLabel_->setText(QString("%1 messages | %2 questions | %3")
        .arg(messages_.size()).arg(questions)
        .arg(paperTitle_.isEmpty() ? "No paper selected" : paperTitle_.left(20)));
}

void PaperQaChatWidget::loadSettings() {
    int size = settings_.beginReadArray("messages");
    for (int i = 0; i < size; ++i) {
        settings_.setArrayIndex(i);
        QaMessage m;
        m.id = settings_.value("id").toInt();
        m.text = settings_.value("text").toString();
        m.sender = settings_.value("sender").toString();
        m.time = QTime::fromString(settings_.value("time").toString(), Qt::ISODate);
        m.paperTitle = settings_.value("paperTitle").toString();
        m.paperId = settings_.value("paperId").toInt();
        messages_.append(m);
    }
    settings_.endArray();
    updateInfo();
}

void PaperQaChatWidget::saveSettings() {
    settings_.beginWriteArray("messages");
    for (int i = 0; i < messages_.size(); ++i) {
        settings_.setArrayIndex(i);
        settings_.setValue("id", messages_[i].id);
        settings_.setValue("text", messages_[i].text);
        settings_.setValue("sender", messages_[i].sender);
        settings_.setValue("time", messages_[i].time.toString(Qt::ISODate));
        settings_.setValue("paperTitle", messages_[i].paperTitle);
        settings_.setValue("paperId", messages_[i].paperId);
    }
    settings_.endArray();
}
