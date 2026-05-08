#include "workspace/CollaborationWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QDateTime>

CollaborationWidget::CollaborationWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void CollaborationWidget::setupUI() {
    auto* mainLayout = new QHBoxLayout(this);

    // Left: sessions list
    auto* leftPanel = new QVBoxLayout();

    auto* header = new QLabel("Collaboration");
    header->setStyleSheet("font-weight: bold; font-size: 14px;");
    leftPanel->addWidget(header);

    auto* btnRow = new QHBoxLayout();
    createBtn_ = new QPushButton("Create");
    createBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 4px 12px; border-radius: 4px; }"
    );
    connect(createBtn_, &QPushButton::clicked, this, &CollaborationWidget::onCreateSession);
    btnRow->addWidget(createBtn_);

    joinBtn_ = new QPushButton("Join");
    connect(joinBtn_, &QPushButton::clicked, this, &CollaborationWidget::onJoinSession);
    btnRow->addWidget(joinBtn_);

    leaveBtn_ = new QPushButton("Leave");
    leaveBtn_->setStyleSheet("color: #dc2626;");
    connect(leaveBtn_, &QPushButton::clicked, this, &CollaborationWidget::onLeaveSession);
    btnRow->addWidget(leaveBtn_);

    leftPanel->addLayout(btnRow);

    sessionList_ = new QListWidget();
    sessionList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
        "QListWidget::item { padding: 6px; }"
        "QListWidget::item:selected { background: #3b82f6; color: white; }"
    );
    connect(sessionList_, &QListWidget::itemClicked, this, &CollaborationWidget::onSessionClicked);
    leftPanel->addWidget(sessionList_, 1);

    mainLayout->addLayout(leftPanel, 1);

    // Right: session detail
    auto* rightPanel = new QVBoxLayout();

    infoLabel_ = new QLabel("Select a session");
    infoLabel_->setStyleSheet("font-weight: bold; font-size: 12px;");
    rightPanel->addWidget(infoLabel_);

    // Members
    rightPanel->addWidget(new QLabel("Members:"));
    memberList_ = new QListWidget();
    memberList_->setMaximumHeight(120);
    memberList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
    );
    rightPanel->addWidget(memberList_);

    // Shared papers
    auto* paperRow = new QHBoxLayout();
    paperRow->addWidget(new QLabel("Shared Papers:"));
    paperRow->addStretch();
    shareBtn_ = new QPushButton("Share Paper");
    connect(shareBtn_, &QPushButton::clicked, this, &CollaborationWidget::onSharePaper);
    paperRow->addWidget(shareBtn_);
    rightPanel->addLayout(paperRow);

    paperList_ = new QListWidget();
    paperList_->setStyleSheet(
        "QListWidget { border: 1px solid palette(mid); border-radius: 4px; }"
    );
    rightPanel->addWidget(paperList_, 1);

    // Message
    auto* msgRow = new QHBoxLayout();
    messageEdit_ = new QLineEdit();
    messageEdit_->setPlaceholderText("Type a message...");
    auto* sendBtn = new QPushButton("Send");
    connect(sendBtn, &QPushButton::clicked, this, &CollaborationWidget::onSendMessage);
    connect(messageEdit_, &QLineEdit::returnPressed, this, &CollaborationWidget::onSendMessage);
    msgRow->addWidget(messageEdit_, 1);
    msgRow->addWidget(sendBtn);
    rightPanel->addLayout(msgRow);

    mainLayout->addLayout(rightPanel, 2);
}

void CollaborationWidget::setSessions(const QList<CollaborationSession>& sessions) {
    sessions_ = sessions;
    nextId_ = 1;
    for (const auto& s : sessions_) nextId_ = qMax(nextId_, s.id + 1);
    refreshSessionList();
}

QList<CollaborationSession> CollaborationWidget::sessions() const {
    return sessions_;
}

void CollaborationWidget::onCreateSession() {
    QString name = QInputDialog::getText(this, "Create Session", "Session name:");
    if (name.trimmed().isEmpty()) return;

    CollaborationSession session;
    session.id = nextId_++;
    session.name = name.trimmed();
    session.owner = "You";
    session.status = "active";
    session.createdAt = QDateTime::currentSecsSinceEpoch();
    sessions_.append(session);
    refreshSessionList();
    emit sessionCreated(session);
}

void CollaborationWidget::onJoinSession() {
    QString code = QInputDialog::getText(this, "Join Session", "Enter session code or name:");
    if (code.trimmed().isEmpty()) return;
    // Placeholder
    emit sessionJoined(0);
}

void CollaborationWidget::onLeaveSession() {
    if (selectedSessionId_ < 0) return;
    auto result = QMessageBox::question(this, "Leave Session", "Leave this session?");
    if (result != QMessageBox::Yes) return;

    emit sessionLeft(selectedSessionId_);
    selectedSessionId_ = -1;
    memberList_->clear();
    paperList_->clear();
    infoLabel_->setText("Select a session");
    refreshSessionList();
}

void CollaborationWidget::onSharePaper() {
    if (selectedSessionId_ < 0) return;
    QString paperTitle = QInputDialog::getText(this, "Share Paper", "Paper title or ID:");
    if (paperTitle.trimmed().isEmpty()) return;

    for (auto& s : sessions_) {
        if (s.id == selectedSessionId_) {
            s.sharedPaperIds.append(paperTitle);
            s.updatedAt = QDateTime::currentSecsSinceEpoch();
            refreshSharedPapers();
            emit paperShared(selectedSessionId_, 0);
            break;
        }
    }
}

void CollaborationWidget::onSendMessage() {
    if (selectedSessionId_ < 0) return;
    QString msg = messageEdit_->text().trimmed();
    if (msg.isEmpty()) return;
    emit messageSent(selectedSessionId_, msg);
    messageEdit_->clear();
}

void CollaborationWidget::onSessionClicked(QListWidgetItem* item) {
    if (!item) return;
    selectedSessionId_ = item->data(Qt::UserRole).toInt();
    refreshMembers();
    refreshSharedPapers();
}

void CollaborationWidget::refreshSessionList() {
    sessionList_->clear();
    for (const auto& s : sessions_) {
        QString display = QString("%1 (%2 members) — %3")
            .arg(s.name).arg(s.members.size()).arg(s.status);
        auto* item = new QListWidgetItem(display);
        item->setData(Qt::UserRole, s.id);
        sessionList_->addItem(item);
    }
}

void CollaborationWidget::refreshMembers() {
    memberList_->clear();
    for (const auto& s : sessions_) {
        if (s.id == selectedSessionId_) {
            infoLabel_->setText(QString("%1 — Owner: %2").arg(s.name, s.owner));
            memberList_->addItem(s.owner + " (owner)");
            for (const auto& m : s.members) {
                memberList_->addItem(m);
            }
            break;
        }
    }
}

void CollaborationWidget::refreshSharedPapers() {
    paperList_->clear();
    for (const auto& s : sessions_) {
        if (s.id == selectedSessionId_) {
            for (const auto& pid : s.sharedPaperIds) {
                paperList_->addItem("Paper: " + pid);
            }
            break;
        }
    }
}
