#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTextEdit>
#include <QNetworkAccessManager>

class DoiLookupDialog : public QDialog {
    Q_OBJECT

public:
    explicit DoiLookupDialog(QWidget* parent = nullptr);

signals:
    void paperFound(const QJsonObject& paperData);

private slots:
    void onLookup();
    void onLookupReply();

private:
    void setupUI();
    void fillFromData(const QJsonObject& data);

    QLineEdit* doiEdit_{nullptr};
    QLabel* statusLabel_{nullptr};
    QLineEdit* titleEdit_{nullptr};
    QLineEdit* authorsEdit_{nullptr};
    QLineEdit* yearEdit_{nullptr};
    QLineEdit* journalEdit_{nullptr};
    QLineEdit* abstractKeywords_{nullptr};
    QTextEdit* abstractEdit_{nullptr};
    QPushButton* lookupBtn_{nullptr};
    QPushButton* importBtn_{nullptr};

    QNetworkAccessManager* networkManager_{nullptr};
    QJsonObject lastResult_;
};
