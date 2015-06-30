/****************************************************************************
** Meta object code from reading C++ file 'rtpclient.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.3.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "../../rtpclient.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'rtpclient.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.3.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
struct qt_meta_stringdata_rtpClient_t {
    QByteArrayData data[13];
    char stringdata[118];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_rtpClient_t, stringdata) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_rtpClient_t qt_meta_stringdata_rtpClient = {
    {
QT_MOC_LITERAL(0, 0, 9),
QT_MOC_LITERAL(1, 10, 7),
QT_MOC_LITERAL(2, 18, 0),
QT_MOC_LITERAL(3, 19, 9),
QT_MOC_LITERAL(4, 29, 5),
QT_MOC_LITERAL(5, 35, 9),
QT_MOC_LITERAL(6, 45, 8),
QT_MOC_LITERAL(7, 54, 12),
QT_MOC_LITERAL(8, 67, 14),
QT_MOC_LITERAL(9, 82, 6),
QT_MOC_LITERAL(10, 89, 6),
QT_MOC_LITERAL(11, 96, 8),
QT_MOC_LITERAL(12, 105, 12)
    },
    "rtpClient\0display\0\0IplImage*\0index\0"
    "heartpack\0getFrame\0sendH264Data\0"
    "unsigned char*\0buffer\0length\0sendData\0"
    "finishDecode"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_rtpClient[] = {

 // content:
       7,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    2,   44,    2, 0x06 /* Public */,
       5,    0,   49,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       6,    0,   50,    2, 0x0a /* Public */,
       7,    2,   51,    2, 0x0a /* Public */,
      11,    0,   56,    2, 0x0a /* Public */,
      12,    2,   57,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3, QMetaType::Int,    2,    4,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8, QMetaType::Int,    9,   10,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8, QMetaType::Int,    9,   10,

       0        // eod
};

void rtpClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        rtpClient *_t = static_cast<rtpClient *>(_o);
        switch (_id) {
        case 0: _t->display((*reinterpret_cast< IplImage*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 1: _t->heartpack(); break;
        case 2: _t->getFrame(); break;
        case 3: _t->sendH264Data((*reinterpret_cast< unsigned char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        case 4: _t->sendData(); break;
        case 5: _t->finishDecode((*reinterpret_cast< unsigned char*(*)>(_a[1])),(*reinterpret_cast< int(*)>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        void **func = reinterpret_cast<void **>(_a[1]);
        {
            typedef void (rtpClient::*_t)(IplImage * , int );
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&rtpClient::display)) {
                *result = 0;
            }
        }
        {
            typedef void (rtpClient::*_t)();
            if (*reinterpret_cast<_t *>(func) == static_cast<_t>(&rtpClient::heartpack)) {
                *result = 1;
            }
        }
    }
}

const QMetaObject rtpClient::staticMetaObject = {
    { &rtpClass::staticMetaObject, qt_meta_stringdata_rtpClient.data,
      qt_meta_data_rtpClient,  qt_static_metacall, 0, 0}
};


const QMetaObject *rtpClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *rtpClient::qt_metacast(const char *_clname)
{
    if (!_clname) return 0;
    if (!strcmp(_clname, qt_meta_stringdata_rtpClient.stringdata))
        return static_cast<void*>(const_cast< rtpClient*>(this));
    return rtpClass::qt_metacast(_clname);
}

int rtpClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = rtpClass::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            *reinterpret_cast<int*>(_a[0]) = -1;
        _id -= 6;
    }
    return _id;
}

// SIGNAL 0
void rtpClient::display(IplImage * _t1, int _t2)
{
    void *_a[] = { 0, const_cast<void*>(reinterpret_cast<const void*>(&_t1)), const_cast<void*>(reinterpret_cast<const void*>(&_t2)) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void rtpClient::heartpack()
{
    QMetaObject::activate(this, &staticMetaObject, 1, 0);
}
QT_END_MOC_NAMESPACE
