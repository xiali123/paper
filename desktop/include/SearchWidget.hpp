#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidget>
#include <QGraphicsDropShadowEffect>

class SearchHistory;

class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);
    QString getKeyword() const { return keywordEdit_->text(); }
    void setPlaceholder(const QString& text);
    void setText(const QString& text);
    void setFocus();
    void setSearchHistory(SearchHistory* history);

signals:
    void searchRequested(const QString& keyword);

private slots:
    void onSearchClicked();
    void onSuggestionClicked();
    void onTextChanged(const QString& text);
    void onHistoryItemClicked(QListWidgetItem* item);

private:
    void setupUI();
    void setupStyles();
    void setupSuggestions();
    void showHistoryDropdown();
    void hideHistoryDropdown();

    QLineEdit* keywordEdit_{nullptr};
    QPushButton* searchButton_{nullptr};
    QLabel* statusLabel_{nullptr};
    QLabel* suggestionsLabel_{nullptr};
    QList<QPushButton*> suggestionButtons_;
    QListWidget* historyDropdown_{nullptr};
    QGraphicsDropShadowEffect* shadowEffect_{nullptr};
    SearchHistory* searchHistory_{nullptr};
};
