#pragma once

#include <QMenu>
#include <QList>

class ApiManager;
class FavoriteManager;
struct Paper;

class ContextMenuBuilder {
public:
    static QMenu* buildPaperMenu(const Paper& paper, ApiManager* apiManager,
                                  FavoriteManager* favManager, QWidget* parent);

    static QMenu* buildSearchResultMenu(const QList<int>& selectedIds,
                                         ApiManager* apiManager, QWidget* parent);

    static QMenu* buildTabMenu(class QTabWidget* tabWidget, QWidget* parent);
};
