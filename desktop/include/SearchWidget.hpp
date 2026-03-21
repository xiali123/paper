#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>

/**
 * @brief Search input widget
 */
class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);
    QString getKeyword() const { return keywordEdit_->text(); }

signals:
    void searchRequested(const QString& keyword);

private slots:
    void onSearchClicked();

private:
    void setupUI();

    QLineEdit* keywordEdit_{nullptr};
    QPushButton* searchButton_{nullptr};
    QLabel* statusLabel_{nullptr};
};
