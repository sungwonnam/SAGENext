#include "interactiveappexample.h"
#include "ui_interactiveappexample.h"

InteractiveAppExample::InteractiveAppExample() :
		ui(new Ui::InteractiveAppExample),
		root(0)
{

	/* init GUI */
	ui->setupUi(this);
	connect(ui->button1, SIGNAL(clicked()), this, SLOT(setLabelText1()));
	connect(ui->button2, SIGNAL(clicked()), this, SLOT(setLabelText2()));

	/*
	  Below is to integrate your GUI application into SAGENext
	  */
	root = new QGraphicsProxyWidget();
	root->setWindowFlags(Qt::Window);
	root->setWindowFrameMargins(3, 30, 3, 3);
	root->setWidget(this);
}

InteractiveAppExample::~InteractiveAppExample()
{
	/**
	  Don't delete 'root' here
	  This will be deleted automatically by SAGENext
	  **/
}

QGraphicsProxyWidget * InteractiveAppExample::rootWidget()
{
	return root;
}


void InteractiveAppExample::setLabelText1()
{
	ui->label->setText("Button 1 clicked !");

}

void InteractiveAppExample::setLabelText2()
{
	ui->label->setText("Button 2 clicked !");
}


QT_BEGIN_NAMESPACE
Q_EXPORT_PLUGIN2(IAppExample, InteractiveAppExample)
QT_END_NAMESPACE
