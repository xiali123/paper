#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QTableWidget>
#include <QHeaderView>
#include <QPushButton>

class CrawlerDashboardDialog : public QDialog {
    Q_OBJECT

public:
    explicit CrawlerDashboardDialog(QWidget* parent = nullptr);

private:
    void setupUI();
    QWidget* createDashboardTab();
    QWidget* createTasksTab();
    QWidget* createTemplatesTab();
};

class AIDialog : public QDialog {
    Q_OBJECT

public:
    explicit AIDialog(QWidget* parent = nullptr);

private:
    void setupUI();
    QWidget* createReviewTab();
    QWidget* createChatTab();
    QWidget* createHistoryTab();
};
