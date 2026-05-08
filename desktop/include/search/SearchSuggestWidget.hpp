#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QTimer>
#include <QList>

class SearchSuggestWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchSuggestWidget(QLineEdit* parentEdit, QWidget* parent = nullptr);

    void setSuggestions(const QStringList& items);
    void setHistory(const QStringList& history);

signals:
    void suggestionSelected(const QString& text);

private slots:
    void onSuggestionClicked(QListWidgetItem* item);
    void onTextChanged();

private:
    void setupUI();
    void showPopup();
    void hidePopup();

    QLineEdit* edit_{nullptr};
    QListWidget* listWidget_{nullptr};
    QTimer* debounceTimer_{nullptr};

    QStringList suggestions_;
    QStringList history_;
    static constexpr int DEBOUNCE_MS = 200;
};
