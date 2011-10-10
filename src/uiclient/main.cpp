#include <QtGui/QApplication>
#include "sn_pointerui.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
//	qDebug() << QCoreApplication::applicationDirPath();
//	qDebug() << QCoreApplication::applicationFilePath();
	
	a.setApplicationName("sagenextPointer");
	SN_PointerUI w;
	w.show();

	return a.exec();
}
