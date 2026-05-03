#include "ContextMenuBuilder.hpp"
#include "PaperTypes.hpp"
#include "ApiManager.hpp"
#include "FavoriteManager.hpp"
#include "PaperNotesDialog.hpp"
#include "RatingWidget.hpp"
#include <QApplication>
#include <QClipboard>
#include <QTabWidget>

QMenu* ContextMenuBuilder::buildPaperMenu(const Paper& paper, ApiManager* apiManager,
                                           FavoriteManager* favManager, QWidget* parent) {
    auto* menu = new QMenu(parent);

    // Open
    auto* openAction = menu->addAction("Open Details");
    connect(openAction, &QAction::triggered, parent, [apiManager, paperId = paper.id]() {
        apiManager->getPaperDetails(paperId);
    });

    menu->addSeparator();

    // Favorite
    auto* favAction = menu->addAction(paper.isFavorite ? "Unfavorite" : "Favorite");
    connect(favAction, &QAction::triggered, parent, [apiManager, paperId = paper.id, fav = paper.isFavorite]() {
        apiManager->togglePaperFavorite(paperId, !fav);
    });

    // Notes
    auto* notesAction = menu->addAction("Add Note");
    connect(notesAction, &QAction::triggered, parent, [parent, paper]() {
        auto* dlg = new PaperNotesDialog(paper.id, paper.title, nullptr, parent);
        dlg->exec();
        dlg->deleteLater();
    });

    menu->addSeparator();

    // Copy
    auto* copyTitle = menu->addAction("Copy Title");
    connect(copyTitle, &QAction::triggered, parent, [title = paper.title]() {
        QApplication::clipboard()->setText(title);
    });

    auto* copyAuthors = menu->addAction("Copy Authors");
    connect(copyAuthors, &QAction::triggered, parent, [authors = paper.authors]() {
        QApplication::clipboard()->setText(authors);
    });

    auto* copyDoi = menu->addAction("Copy DOI");
    copyDoi->setEnabled(!paper.doiUrl.isEmpty());
    connect(copyDoi, &QAction::triggered, parent, [doi = paper.doiUrl]() {
        QApplication::clipboard()->setText(doi);
    });

    auto* copyCite = menu->addAction("Copy Citation");
    connect(copyCite, &QAction::triggered, parent, [paper]() {
        QString cite = QString("%1 (%2). %3. %4")
            .arg(paper.authors, paper.year,
                 paper.title,
                 paper.journalFull.isEmpty() ? paper.journal : paper.journalFull);
        QApplication::clipboard()->setText(cite);
    });

    menu->addSeparator();

    // Tags
    auto* tagsAction = menu->addAction("Manage Tags");
    connect(tagsAction, &QAction::triggered, parent, [apiManager, paperId = paper.id]() {
        Q_UNUSED(apiManager);
        Q_UNUSED(paperId);
    });

    // Mark read
    auto* readAction = menu->addAction("Mark as Read");
    connect(readAction, &QAction::triggered, parent, [apiManager, paperId = paper.id]() {
        apiManager->markPaperRead(paperId);
    });

    menu->addSeparator();

    // Delete
    auto* deleteAction = menu->addAction("Delete");
    deleteAction->setStyleSheet("color: #dc2626;");
    connect(deleteAction, &QAction::triggered, parent, [apiManager, paperId = paper.id]() {
        apiManager->deletePaper(paperId);
    });

    return menu;
}

QMenu* ContextMenuBuilder::buildSearchResultMenu(const QList<int>& selectedIds,
                                                  ApiManager* apiManager, QWidget* parent) {
    auto* menu = new QMenu(parent);

    auto* favAction = menu->addAction(QString("Favorite %1 papers").arg(selectedIds.size()));
    connect(favAction, &QAction::triggered, parent, [apiManager, selectedIds]() {
        for (int id : selectedIds) apiManager->togglePaperFavorite(id, true);
    });

    auto* unfavAction = menu->addAction(QString("Unfavorite %1 papers").arg(selectedIds.size()));
    connect(unfavAction, &QAction::triggered, parent, [apiManager, selectedIds]() {
        for (int id : selectedIds) apiManager->togglePaperFavorite(id, false);
    });

    menu->addSeparator();

    auto* deleteAction = menu->addAction(QString("Delete %1 papers").arg(selectedIds.size()));
    deleteAction->setStyleSheet("color: #dc2626;");
    connect(deleteAction, &QAction::triggered, parent, [apiManager, selectedIds]() {
        for (int id : selectedIds) apiManager->deletePaper(id);
    });

    return menu;
}

QMenu* ContextMenuBuilder::buildTabMenu(QTabWidget* tabWidget, QWidget* parent) {
    auto* menu = new QMenu(parent);

    for (int i = 0; i < tabWidget->count(); ++i) {
        auto* action = menu->addAction(tabWidget->tabText(i));
        connect(action, &QAction::triggered, parent, [tabWidget, i]() {
            tabWidget->setCurrentIndex(i);
        });
    }

    return menu;
}
