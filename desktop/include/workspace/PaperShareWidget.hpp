#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTextEdit>
#include <QComboBox>
#include <QListWidget>
#include <QList>
#include <QSettings>

struct ShareRecord {
    int id{-1};
    int paperId{-1};
    QString method;
    QString recipient;
    QString message;
    qint64 timestamp{0};
};

class PaperShareWidget : public QWidget {
    Q_OBJECT

public:
    explicit PaperShareWidget(QWidget* parent = nullptr);

    void setPaper(int paperId, const QString& title, const QString& doi = "");
    void shareViaLink();
    void shareViaEmail(const QString& to, const QString& subject, const QString& body);
    void shareViaClipboard(const QString& format);
    QList<ShareRecord> history() const;

signals:
    void shared(int paperId, const QString& method);
    void linkCopied(const QString& link);

private slots:
    void onShare();
    void onCopyLink();
    void onCopyCitation();
    void onCopyBibTeX();
    void onClearHistory();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void refreshHistory();
    void addRecord(const QString& method, const QString& recipient, const QString& message);

    QLabel* paperLabel_{nullptr};
    QLineEdit* recipientEdit_{nullptr};
    QTextEdit* messageEdit_{nullptr};
    QComboBox* methodCombo_{nullptr};
    QPushButton* shareBtn_{nullptr};
    QPushButton* copyLinkBtn_{nullptr};
    QPushButton* copyCiteBtn_{nullptr};
    QPushButton* copyBibBtn_{nullptr};
    QListWidget* historyList_{nullptr};
    QLabel* statusLabel_{nullptr};

    int currentPaperId_{-1};
    QString currentTitle_;
    QString currentDoi_;
    QList<ShareRecord> records_;
    int nextId_{1};
};
