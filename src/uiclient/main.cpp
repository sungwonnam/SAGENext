#include <QtGui/QApplication>
#include "externalguimain.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);
	
//	qDebug() << QCoreApplication::applicationDirPath();
//	qDebug() << QCoreApplication::applicationFilePath();
	
	a.setApplicationName("sagenextPointer");
	ExternalGUIMain w;
	w.show();

	return a.exec();
}
