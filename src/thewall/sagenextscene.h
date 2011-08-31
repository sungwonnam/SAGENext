#ifndef SAGENEXTSCENE_H
#define SAGENEXTSCENE_H

#include <QGraphicsScene>


class SAGENextScene : public QGraphicsScene
{
	Q_OBJECT
public:
	explicit SAGENextScene(QObject *parent = 0);
	~SAGENextScene();


private:


signals:

public slots:
	void closeAllUserApp();

};

#endif // SAGENEXTSCENE_H
