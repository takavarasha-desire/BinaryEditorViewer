#include <QApplication>
#include <QCommandLineParser>


#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Binary Editor");

    QCommandLineParser parser;
    parser.addPositionalArgument("file", "File to open");
    parser.addHelpOption();
    parser.process(app);
    MainWindow *mainWindow = new MainWindow;
    if(!parser.positionalArguments().isEmpty())
        mainWindow->loadFile(parser.positionalArguments().at(0));
    mainWindow->show();

    return app.exec();
}
