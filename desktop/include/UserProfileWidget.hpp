#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QMap>

struct UserProfile {
    int id{-1};
    QString username;
    QString email;
    QString displayName;
    QString role;
    QString avatar;
    QString bio;
    QStringList interests;
    QMap<QString, QVariant> preferences;
    qint64 joinedAt{0};
    qint64 lastLogin{0};
    int papersRead{0};
    int papersFavorited{0};
};

class UserProfileWidget : public QWidget {
    Q_OBJECT

public:
    explicit UserProfileWidget(QWidget* parent = nullptr);

    void setProfile(const UserProfile& profile);
    UserProfile profile() const;

    void setReadOnly(bool ro);
    bool isReadOnly() const { return readOnly_; }

    void loadFromSettings();
    void saveToSettings();

signals:
    void profileUpdated(const UserProfile& profile);
    void avatarChangeRequested();
    void passwordChangeRequested();

private slots:
    void onSave();
    void onCancel();
    void onChangeAvatar();
    void onChangePassword();

private:
    void setupUI();
    void populateFields();
    void updateStats();

    QLineEdit* usernameEdit_{nullptr};
    QLineEdit* emailEdit_{nullptr};
    QLineEdit* displayNameEdit_{nullptr};
    QTextEdit* bioEdit_{nullptr};
    QLabel* avatarLabel_{nullptr};
    QLabel* statsLabel_{nullptr};
    QComboBox* roleCombo_{nullptr};
    QPushButton* saveBtn_{nullptr};
    QPushButton* cancelBtn_{nullptr};

    UserProfile profile_;
    bool readOnly_{false};
};
