#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QTabWidget>
#include <QList>

class MiniBrowserWidget : public QWidget {
    Q_OBJECT

public:
    explicit MiniBrowserWidget(QWidget* parent = nullptr);

    void loadUrl(const QString& url);
    void loadHtml(const QString& html, const QString& baseUrl = "");
    void setSearchEngine(const QString& engine);

    QString currentUrl() const { return currentUrl_; }
    QStringList history() const { return history_; }

signals:
    void urlChanged(const QString& url);
    void titleChanged(const QString& title);
    void loadFinished(bool ok);
    void linkClicked(const QString& url);

private slots:
    void onGo();
    void onBack();
    void onForward();
    void onRefresh();
    void onUrlReturnPressed();

private:
    void setupUI();
    void updateNavButtons();
    void addToHistory(const QString& url);

    QLineEdit* urlBar_{nullptr};
    QLabel* contentLabel_{nullptr};
    QLabel* statusLabel_{nullptr};
    QPushButton* backBtn_{nullptr};
    QPushButton* forwardBtn_{nullptr};
    QPushButton* refreshBtn_{nullptr};
    QPushButton* goBtn_{nullptr};

    QString currentUrl_;
    QString searchEngine_{"https://scholar.google.com/scholar?q="};
    QStringList history_;
    int historyIndex_{-1};
};
