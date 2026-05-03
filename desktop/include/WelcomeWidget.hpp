#pragma once

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>

class WelcomeWidget : public QWidget {
    Q_OBJECT

public:
    explicit WelcomeWidget(QWidget* parent = nullptr);

    void setRecentPapers(const QList<QPair<int, QString>>& papers);
    void setTip(const QString& tip);

signals:
    void searchRequested(const QString& query);
    void openPaperRequested(int paperId);
    void openTabRequested(int tab);
    void tourRequested();

private:
    void setupUI();

    QLabel* welcomeLabel_{nullptr};
    QListWidget* recentList_{nullptr};
    QListWidget* tipList_{nullptr};
    QPushButton* searchBtn_{nullptr};
    QPushButton* tourBtn_{nullptr};
};
