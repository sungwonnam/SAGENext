#ifndef BINARYSEARCHTREE_H
#define BINARYSEARCHTREE_H

#include <QObject>

class BinarySearchTree : public QObject
{
	Q_OBJECT
public:
	explicit BinarySearchTree(QObject *parent = 0);

protected:
	void *_RootNode;

signals:

public slots:

};

#endif // BINARYSEARCHTREE_H
