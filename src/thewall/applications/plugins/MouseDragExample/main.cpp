#include <QtGui/QApplication>
#include "mousedragexample.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MouseDragExample w;
    w.show();

    return a.exec();
}
