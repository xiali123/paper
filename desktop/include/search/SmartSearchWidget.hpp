#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QMap>
#include <QList>

struct SearchToken {
    QString type;   // "keyword", "author", "year", "journal", "doi", "field"
    QString value;
    bool negated{false};
};

class SmartSearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SmartSearchWidget(QWidget* parent = nullptr);

    void setSearchHistory(const QStringList& history);
    QStringList searchHistory() const;

    void setSavedSearches(const QList<QPair<QString, QString>>& searches);
    void addSavedSearch(const QString& name, const QString& query);

    QList<SearchToken> parseQuery(const QString& query) const;
    QString buildQuery(const QList<SearchToken>& tokens) const;

    QStringList suggestions(const QString& partial) const;

signals:
    void searchRequested(const QString& query);
    void smartSearchRequested(const QList<SearchToken>& tokens);
    void searchSaved(const QString& name, const QString& query);

private slots:
    void onSearch();
    void onClear();
    void onSaveSearch();
    void onLoadSearch();
    void onSuggestionClicked(QListWidgetItem* item);
    void onTextChanged(const QString& text);

private:
    void setupUI();
    void updateSuggestions(const QString& text);
    void addToHistory(const QString& query);

    QLineEdit* searchEdit_{nullptr};
    QListWidget* suggestList_{nullptr};
    QListWidget* tokenList_{nullptr};
    QComboBox* fieldCombo_{nullptr};
    QLabel* parsedLabel_{nullptr};
    QLabel* statsLabel_{nullptr};

    QStringList history_;
    QList<QPair<QString, QString>> savedSearches_;
    QMap<QString, QStringList> fieldValues_; // field -> known values

    static constexpr int MAX_HISTORY = 50;
};
