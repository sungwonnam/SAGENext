#include <QtGui/QApplication>
#include "externalguimain.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	a.setApplicationName("sagenext external GUI");
	ExternalGUIMain w;
	w.show();

	return a.exec();
}
