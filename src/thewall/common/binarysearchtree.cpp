#include "binarysearchtree.h"

#include "../applications/base/basewidget.h"

BinarySearchTree::BinarySearchTree(QObject *parent) :
	QObject(parent)
{
}

BinarySearchTree::~BinarySearchTree() {

}

void BinarySearchTree::insertNode(BaseWidget *&treeNode, BaseWidget *newnode) {
	if (treeNode == 0)
		treeNode = newnode;
	else if (newnode->priority()

}
