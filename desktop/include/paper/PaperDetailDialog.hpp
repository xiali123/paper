#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include "core/PaperTypes.hpp"

class FavoriteManager;
class ApiManager;

class PaperDetailDialog : public QDialog {
    Q_OBJECT

public:
    explicit PaperDetailDialog(const Paper& paper, FavoriteManager* favMgr = nullptr,
                               ApiManager* apiMgr = nullptr, QWidget* parent = nullptr);

signals:
    void favoriteToggled(int paperId, bool favorite);
    void paperDeleted(int paperId);
    void tagsChanged(int paperId, const QStringList& tags);

private:
    void setupUI(const Paper& paper);
    QWidget* createInfoRow(const QString& label, const QString& value);
    QWidget* createBadge(const QString& text, const QString& color);
    void updateFavoriteButton();
    void updateTagsDisplay();

    Paper paper_;
    FavoriteManager* favManager_{nullptr};
    ApiManager* apiManager_{nullptr};
    QPushButton* favBtn_{nullptr};
    QWidget* tagsContainer_{nullptr};
    QHBoxLayout* tagsLayout_{nullptr};
    QStringList currentTags_;
};
