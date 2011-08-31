/****************************************************************************
** Meta object code from reading C++ file 'externalguimain.h'
**
** Created: Wed Apr 13 16:45:36 2011
**      by: The Qt Meta Object Compiler version 62 (Qt 4.7.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../externalguimain.h"
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'externalguimain.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 62
#error "This file was generated using the moc from 4.7.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
static const uint qt_meta_data_ExternalGUIMain[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      17,   16,   16,   16, 0x08,
      53,   16,   16,   16, 0x08,
      76,   16,   16,   16, 0x08,
     103,   16,   16,   16, 0x08,
     117,   16,   16,   16, 0x08,
     149,   16,   16,   16, 0x08,
     179,  172,   16,   16, 0x08,
     203,   16,   16,   16, 0x08,
     263,  252,  233,   16, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ExternalGUIMain[] = {
    "ExternalGUIMain\0\0on_actionNew_Connection_triggered()\0"
    "on_vncButton_clicked()\0"
    "on_pointerButton_clicked()\0ungrabMouse()\0"
    "on_actionOpen_Media_triggered()\0"
    "readFiles(QStringList)\0layout\0"
    "updateScene(QByteArray)\0"
    "on_showLayoutButton_clicked()\0"
    "QGraphicsRectItem*\0scene,gaid\0"
    "itemWithGlobalAppId(QGraphicsScene*,quint64)\0"
};

const QMetaObject ExternalGUIMain::staticMetaObject = {
    { &QMainWindow::staticMetaObject, qt_meta_stringdata_ExternalGUIMain,
      qt_meta_data_ExternalGUIMain, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ExternalGUIMain::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ExternalGUIMain::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ExternalGUIMain::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ExternalGUIMain))
        return static_cast<void*>(const_cast< ExternalGUIMain*>(this));
    return QMainWindow::qt_metacast(_clname);
}

int ExternalGUIMain::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: on_actionNew_Connection_triggered(); break;
        case 1: on_vncButton_clicked(); break;
        case 2: on_pointerButton_clicked(); break;
        case 3: ungrabMouse(); break;
        case 4: on_actionOpen_Media_triggered(); break;
        case 5: readFiles((*reinterpret_cast< QStringList(*)>(_a[1]))); break;
        case 6: updateScene((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 7: on_showLayoutButton_clicked(); break;
        case 8: { QGraphicsRectItem* _r = itemWithGlobalAppId((*reinterpret_cast< QGraphicsScene*(*)>(_a[1])),(*reinterpret_cast< quint64(*)>(_a[2])));
            if (_a[0]) *reinterpret_cast< QGraphicsRectItem**>(_a[0]) = _r; }  break;
        default: ;
        }
        _id -= 9;
    }
    return _id;
}
static const uint qt_meta_data_ConnectionDialog[] = {

 // content:
       5,       // revision
       0,       // classname
       0,    0, // classinfo
       2,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: signature, parameters, type, tag, flags
      18,   17,   17,   17, 0x08,
      42,   17,   17,   17, 0x08,

       0        // eod
};

static const char qt_meta_stringdata_ConnectionDialog[] = {
    "ConnectionDialog\0\0on_buttonBox_rejected()\0"
    "on_buttonBox_accepted()\0"
};

const QMetaObject ConnectionDialog::staticMetaObject = {
    { &QDialog::staticMetaObject, qt_meta_stringdata_ConnectionDialog,
      qt_meta_data_ConnectionDialog, 0 }
};

#ifdef Q_NO_DATA_RELOCATION
const QMetaObject &ConnectionDialog::getStaticMetaObject() { return staticMetaObject; }
#endif //Q_NO_DATA_RELOCATION

const QMetaObject *ConnectionDialog::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->metaObject : &staticMetaObject;
}

void *ConnectionDialog::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_ConnectionDialog))
        return static_cast<void*>(const_cast< ConnectionDialog*>(this));
    return QDialog::qt_metacast(_clname);
}

int ConnectionDialog::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QDialog::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        switch (_id) {
        case 0: on_buttonBox_rejected(); break;
        case 1: on_buttonBox_accepted(); break;
        default: ;
        }
        _id -= 2;
    }
    return _id;
}
QT_END_MOC_NAMESPACE
