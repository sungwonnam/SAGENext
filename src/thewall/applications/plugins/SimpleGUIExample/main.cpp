#include <QtGui/QApplication>
#include "imagewidgetplugin.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ImageWidgetPlugin w;
    w.show();

    return a.exec();
}
