#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include "PaperTypes.hpp"

class FavoriteManager;

class PaperDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaperDetailDialog(const Paper& paper, FavoriteManager* favMgr = nullptr, QWidget* parent = nullptr);

signals:
    void favoriteToggled(int paperId, bool favorite);

private:
    void setupUI(const Paper& paper);
    QWidget* createInfoRow(const QString& label, const QString& value);
    QWidget* createBadge(const QString& text, const QString& color);
    void updateFavoriteButton();

    Paper paper_;
    FavoriteManager* favManager_{nullptr};
    QPushButton* favBtn_{nullptr};
};
