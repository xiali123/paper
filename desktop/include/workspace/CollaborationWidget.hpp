#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QList>
#include <QMap>

struct CollaborationSession {
    int id{-1};
    QString name;
    QString owner;
    QStringList members;
    QStringList sharedPaperIds;
    QString status; // "active", "archived"
    qint64 createdAt{0};
    qint64 updatedAt{0};
};

class CollaborationWidget : public QWidget {
    Q_OBJECT

public:
    explicit CollaborationWidget(QWidget* parent = nullptr);

    void setSessions(const QList<CollaborationSession>& sessions);
    QList<CollaborationSession> sessions() const;

signals:
    void sessionCreated(const CollaborationSession& session);
    void sessionJoined(int sessionId);
    void sessionLeft(int sessionId);
    void paperShared(int sessionId, int paperId);
    void paperUnshared(int sessionId, int paperId);
    void messageSent(int sessionId, const QString& message);

private slots:
    void onCreateSession();
    void onJoinSession();
    void onLeaveSession();
    void onSharePaper();
    void onSendMessage();
    void onSessionClicked(QListWidgetItem* item);

private:
    void setupUI();
    void refreshSessionList();
    void refreshMembers();
    void refreshSharedPapers();

    QListWidget* sessionList_{nullptr};
    QListWidget* memberList_{nullptr};
    QListWidget* paperList_{nullptr};
    QLineEdit* messageEdit_{nullptr};
    QLabel* infoLabel_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* joinBtn_{nullptr};
    QPushButton* leaveBtn_{nullptr};
    QPushButton* shareBtn_{nullptr};

    QList<CollaborationSession> sessions_;
    int selectedSessionId_{-1};
    int nextId_{1};
};
