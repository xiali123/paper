#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>

/**
 * @brief Modern search input widget with glass morphism
 *
 * Features:
 * - Rounded search bar with shadow
 * - Modern gradient search button
 * - Smooth animations
 * - Placeholder hints
 * - Popular search suggestions
 */
class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);
    QString getKeyword() const { return keywordEdit_->text(); }
    void setPlaceholder(const QString& text);
    void setFocus();

signals:
    void searchRequested(const QString& keyword);

private slots:
    void onSearchClicked();
    void onSuggestionClicked();

private:
    void setupUI();
    void setupStyles();
    void setupSuggestions();

    QLineEdit* keywordEdit_{nullptr};
    QPushButton* searchButton_{nullptr};
    QLabel* statusLabel_{nullptr};
    QLabel* suggestionsLabel_{nullptr};
    QList<QPushButton*> suggestionButtons_;
    QGraphicsDropShadowEffect* shadowEffect_{nullptr};
};
