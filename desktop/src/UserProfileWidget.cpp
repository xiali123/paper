#include "UserProfileWidget.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDateTime>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFileDialog>

UserProfileWidget::UserProfileWidget(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
    loadFromSettings();
}

void UserProfileWidget::setupUI() {
    auto* layout = new QVBoxLayout(this);

    // Avatar + stats header
    auto* headerRow = new QHBoxLayout();

    avatarLabel_ = new QLabel();
    avatarLabel_->setFixedSize(64, 64);
    avatarLabel_->setAlignment(Qt::AlignCenter);
    avatarLabel_->setStyleSheet(
        "QLabel { background: #3b82f6; color: white; border-radius: 32px; "
        "font-size: 24px; font-weight: bold; }"
    );
    avatarLabel_->setText("U");
    headerRow->addWidget(avatarLabel_);

    auto* headerInfo = new QVBoxLayout();
    auto* nameHeader = new QLabel("User Profile");
    nameHeader->setStyleSheet("font-weight: bold; font-size: 16px;");
    headerInfo->addWidget(nameHeader);

    statsLabel_ = new QLabel("0 papers read | 0 favorites");
    statsLabel_->setStyleSheet("font-size: 11px; color: #64748b;");
    headerInfo->addWidget(statsLabel_);
    headerRow->addLayout(headerInfo, 1);

    auto* avatarBtn = new QPushButton("Change");
    avatarBtn->setStyleSheet("font-size: 10px;");
    connect(avatarBtn, &QPushButton::clicked, this, &UserProfileWidget::onChangeAvatar);
    headerRow->addWidget(avatarBtn);

    layout->addLayout(headerRow);

    // Form
    auto* formGroup = new QGroupBox("Profile Details");
    auto* form = new QFormLayout(formGroup);

    usernameEdit_ = new QLineEdit();
    form->addRow("Username:", usernameEdit_);

    emailEdit_ = new QLineEdit();
    emailEdit_->setPlaceholderText("email@example.com");
    form->addRow("Email:", emailEdit_);

    displayNameEdit_ = new QLineEdit();
    form->addRow("Display Name:", displayNameEdit_);

    roleCombo_ = new QComboBox();
    roleCombo_->addItems({"Researcher", "Student", "Professor", "Librarian", "Other"});
    form->addRow("Role:", roleCombo_);

    form->addRow("Bio:", bioEdit_);
    bioEdit_ = new QTextEdit();
    bioEdit_->setMaximumHeight(80);
    bioEdit_->setPlaceholderText("Tell us about your research interests...");

    layout->addWidget(formGroup);

    // Buttons
    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    cancelBtn_ = new QPushButton("Cancel");
    connect(cancelBtn_, &QPushButton::clicked, this, &UserProfileWidget::onCancel);
    btnRow->addWidget(cancelBtn_);

    saveBtn_ = new QPushButton("Save");
    saveBtn_->setStyleSheet(
        "QPushButton { background: #3b82f6; color: white; padding: 8px 20px; "
        "border-radius: 6px; font-weight: bold; }"
    );
    connect(saveBtn_, &QPushButton::clicked, this, &UserProfileWidget::onSave);
    btnRow->addWidget(saveBtn_);

    auto* pwBtn = new QPushButton("Change Password");
    connect(pwBtn, &QPushButton::clicked, this, &UserProfileWidget::onChangePassword);
    btnRow->addWidget(pwBtn);

    layout->addLayout(btnRow);
}

void UserProfileWidget::setProfile(const UserProfile& profile) {
    profile_ = profile;
    populateFields();
    updateStats();
}

UserProfile UserProfileWidget::profile() const {
    return profile_;
}

void UserProfileWidget::setReadOnly(bool ro) {
    readOnly_ = ro;
    usernameEdit_->setReadOnly(ro);
    emailEdit_->setReadOnly(ro);
    displayNameEdit_->setReadOnly(ro);
    bioEdit_->setReadOnly(ro);
    roleCombo_->setEnabled(!ro);
    saveBtn_->setVisible(!ro);
    cancelBtn_->setVisible(!ro);
}

void UserProfileWidget::loadFromSettings() {
    QSettings settings("PaperCrawler", "UserProfile");
    profile_.username = settings.value("username").toString();
    profile_.email = settings.value("email").toString();
    profile_.displayName = settings.value("displayName").toString();
    profile_.role = settings.value("role").toString("Researcher");
    profile_.bio = settings.value("bio").toString();
    profile_.avatar = settings.value("avatar").toString();
    profile_.joinedAt = settings.value("joinedAt").toLongLong();
    profile_.papersRead = settings.value("papersRead").toInt();
    profile_.papersFavorited = settings.value("papersFavorited").toInt();
    populateFields();
    updateStats();
}

void UserProfileWidget::saveToSettings() {
    QSettings settings("PaperCrawler", "UserProfile");
    settings.setValue("username", profile_.username);
    settings.setValue("email", profile_.email);
    settings.setValue("displayName", profile_.displayName);
    settings.setValue("role", profile_.role);
    settings.setValue("bio", profile_.bio);
    settings.setValue("avatar", profile_.avatar);
    settings.setValue("joinedAt", profile_.joinedAt);
    settings.setValue("papersRead", profile_.papersRead);
    settings.setValue("papersFavorited", profile_.papersFavorited);
}

void UserProfileWidget::populateFields() {
    usernameEdit_->setText(profile_.username);
    emailEdit_->setText(profile_.email);
    displayNameEdit_->setText(profile_.displayName);
    bioEdit_->setPlainText(profile_.bio);

    int idx = roleCombo_->findText(profile_.role);
    if (idx >= 0) roleCombo_->setCurrentIndex(idx);

    if (!profile_.displayName.isEmpty()) {
        avatarLabel_->setText(profile_.displayName.left(1).toUpper());
    }
}

void UserProfileWidget::updateStats() {
    statsLabel_->setText(QString("%1 papers read | %2 favorites | Joined: %3")
        .arg(profile_.papersRead)
        .arg(profile_.papersFavorited)
        .arg(profile_.joinedAt > 0
            ? QDateTime::fromSecsSinceEpoch(profile_.joinedAt).toString("yyyy-MM-dd")
            : "N/A"));
}

void UserProfileWidget::onSave() {
    profile_.username = usernameEdit_->text().trimmed();
    profile_.email = emailEdit_->text().trimmed();
    profile_.displayName = displayNameEdit_->text().trimmed();
    profile_.role = roleCombo_->currentText();
    profile_.bio = bioEdit_->toPlainText();

    if (profile_.joinedAt == 0) {
        profile_.joinedAt = QDateTime::currentSecsSinceEpoch();
    }

    updateStats();
    saveToSettings();
    emit profileUpdated(profile_);
}

void UserProfileWidget::onCancel() {
    populateFields();
}

void UserProfileWidget::onChangeAvatar() {
    QString path = QFileDialog::getOpenFileName(this, "Select Avatar", "",
        "Images (*.png *.jpg *.jpeg)");
    if (!path.isEmpty()) {
        profile_.avatar = path;
        avatarLabel_->setStyleSheet(
            QString("QLabel { background: #3b82f6; color: white; border-radius: 32px; "
                    "font-size: 24px; font-weight: bold; }")
        );
        emit avatarChangeRequested();
    }
}

void UserProfileWidget::onChangePassword() {
    emit passwordChangeRequested();
}
