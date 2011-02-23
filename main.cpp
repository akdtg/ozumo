#include <QtGui/QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef WIN32
    a.setStyle("Plastique");
#endif
    MainWindow w;
    w.show();

    return a.exec();
}
