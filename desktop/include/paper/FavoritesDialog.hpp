#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>

class FavoriteManager;

class FavoritesDialog : public QDialog {
    Q_OBJECT

public:
    explicit FavoritesDialog(FavoriteManager* favoriteManager, QWidget* parent = nullptr);

private:
    void setupUI();
    void loadFavorites();
    QWidget* createFavoriteItem(const QString& title, const QString& journal,
                                const QString& year, int paperId);

    FavoriteManager* favoriteManager_;
    QVBoxLayout* listLayout_{nullptr};
    QLineEdit* searchEdit_{nullptr};
    QLabel* countLabel_{nullptr};
};
