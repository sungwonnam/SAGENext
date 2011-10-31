/****************************************************************************
** Meta object code from reading C++ file 'imagewidgetplugin.h'
**
** Created: Tue Nov 9 20:46:45 2010
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.0)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "imagewidgetplugin.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'imagewidgetplugin.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.0. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_BaseWidgetPlugin[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_BaseWidgetPlugin[] = {
    "BaseWidgetPlugin\0"
};

const QMetaObject BaseWidgetPlugin::staticMetaObject = {
    { &QGraphicsWidget::staticMetaObject, qt_meta_stringdata_BaseWidgetPlugin,
      qt_meta_data_BaseWidgetPlugin, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &BaseWidgetPlugin::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *BaseWidgetPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *BaseWidgetPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_BaseWidgetPlugin))
        return static_cast<void*>(const_cast< BaseWidgetPlugin*>(this));
    if (!strcmp(_clname, "DummyPluginInterface"))
        return static_cast< DummyPluginInterface*>(const_cast< BaseWidgetPlugin*>(this));
    if (!strcmp(_clname, "edu.uic.evl.Sungwon.SageNext.DummyInterface/0.1"))
        return static_cast< DummyPluginInterface*>(const_cast< BaseWidgetPlugin*>(this));
    return QGraphicsWidget::qt_metacast(_clname);
}

int BaseWidgetPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QGraphicsWidget::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
static const uint qt_meta_data_Example[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       0,    0, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

       0        // eod
};

static const char qt_meta_stringdata_Example[] = {
    "Example\0"
};

const QMetaObject Example::staticMetaObject = {
    { &BaseWidgetPlugin::staticMetaObject, qt_meta_stringdata_Example,
      qt_meta_data_Example, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &Example::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *Example::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *Example::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_Example))
        return static_cast<void*>(const_cast< Example*>(this));
    return BaseWidgetPlugin::qt_metacast(_clname);
}

int Example::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = BaseWidgetPlugin::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    return _id;
}
QT_END_MOC_NAMESPACE
