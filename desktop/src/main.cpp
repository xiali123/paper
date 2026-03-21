#include <QApplication>
#include "MainWindow.hpp"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setApplicationName("PaperCrawler");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("PaperCrawler Project");

    MainWindow window;
    window.show();

    return app.exec();
}
