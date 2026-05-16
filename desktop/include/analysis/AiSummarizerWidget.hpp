#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QProgressBar>
#include <QList>

class AiSummarizerWidget : public QWidget {
    Q_OBJECT

public:
    explicit AiSummarizerWidget(QWidget* parent = nullptr);

    void setPaperContent(const QString& title, const QString& abstract, const QString& fullText);
    void setApiManager(class ApiManager* api) { apiManager_ = api; }

signals:
    void summaryReady(const QString& summary);
    void keywordsReady(const QStringList& keywords);
    void errorOccurred(const QString& error);

private slots:
    void onGenerate();
    void onCopyResult();
    void onTypeChanged(int index);

private:
    void setupUI();

    QPlainTextEdit* inputEdit_{nullptr};
    QPlainTextEdit* resultEdit_{nullptr};
    QComboBox* typeCombo_{nullptr};
    QProgressBar* progressBar_{nullptr};
    QLabel* statusLabel_{nullptr};
    QPushButton* generateBtn_{nullptr};
    QPushButton* copyBtn_{nullptr};

    QString paperTitle_;
    QString paperAbstract_;
    class ApiManager* apiManager_{nullptr};
};
