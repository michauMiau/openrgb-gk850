/****************************************************************************
** Meta object code from reading C++ file 'OpenGK850WPluginV2.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "OpenGK850WPluginV2.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'OpenGK850WPluginV2.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.8.2. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

#ifndef Q_CONSTINIT
#define Q_CONSTINIT
#endif

QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
QT_WARNING_DISABLE_GCC("-Wuseless-cast")
namespace {
struct qt_meta_tag_ZN18OpenGK850WPluginV2E_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN18OpenGK850WPluginV2E = QtMocHelpers::stringData(
    "OpenGK850WPluginV2"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN18OpenGK850WPluginV2E[] = {

 // content:
      12,       // revision
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

Q_CONSTINIT const QMetaObject OpenGK850WPluginV2::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN18OpenGK850WPluginV2E.offsetsAndSizes,
    qt_meta_data_ZN18OpenGK850WPluginV2E,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN18OpenGK850WPluginV2E_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<OpenGK850WPluginV2, std::true_type>
    >,
    nullptr
} };

void OpenGK850WPluginV2::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<OpenGK850WPluginV2 *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *OpenGK850WPluginV2::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OpenGK850WPluginV2::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN18OpenGK850WPluginV2E.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "OpenRGBPluginInterface"))
        return static_cast< OpenRGBPluginInterface*>(this);
    if (!strcmp(_clname, "org.openrgb.OpenRGBPluginInterface"))
        return static_cast< OpenRGBPluginInterface*>(this);
    return QObject::qt_metacast(_clname);
}

int OpenGK850WPluginV2::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_OpenGK850WPluginV2[] = {
    0xbf, 
    // "IID"
    0x02,  0x78,  0x22,  'o',  'r',  'g',  '.',  'o', 
    'p',  'e',  'n',  'r',  'g',  'b',  '.',  'O', 
    'p',  'e',  'n',  'R',  'G',  'B',  'P',  'l', 
    'u',  'g',  'i',  'n',  'I',  'n',  't',  'e', 
    'r',  'f',  'a',  'c',  'e', 
    // "className"
    0x03,  0x72,  'O',  'p',  'e',  'n',  'G',  'K', 
    '8',  '5',  '0',  'W',  'P',  'l',  'u',  'g', 
    'i',  'n',  'V',  '2', 
    // "MetaData"
    0x04,  0xa7,  0x66,  'C',  'o',  'm',  'm',  'i', 
    't',  0x60,  0x6b,  'D',  'e',  's',  'c',  'r', 
    'i',  'p',  't',  'i',  'o',  'n',  0x78,  0x49, 
    'V',  'i',  'r',  't',  'u',  'a',  'l',  ' ', 
    'c',  'o',  'n',  't',  'r',  'o',  'l',  'l', 
    'e',  'r',  ' ',  'p',  'l',  'u',  'g',  'i', 
    'n',  ' ',  'f',  'o',  'r',  ' ',  'G',  'K', 
    '8',  '5',  '0',  'W',  ' ',  'k',  'e',  'y', 
    'b',  'o',  'a',  'r',  'd',  ' ',  'w',  'i', 
    't',  'h',  ' ',  's',  'o',  'u',  'n',  'd', 
    ' ',  'r',  'e',  'a',  'c',  't',  'i',  'v', 
    'e',  ' ',  'e',  'f',  'f',  'e',  'c',  't', 
    's',  0x62,  'I',  'd',  0x72,  'O',  'p',  'e', 
    'n',  'G',  'K',  '8',  '5',  '0',  'W',  'P', 
    'l',  'u',  'g',  'i',  'n',  'V',  '2',  0x64, 
    'N',  'a',  'm',  'e',  0x78,  0x20,  'G',  'K', 
    '8',  '5',  '0',  'W',  ' ',  'V',  'i',  'r', 
    't',  'u',  'a',  'l',  ' ',  'C',  'o',  'n', 
    't',  'r',  'o',  'l',  'l',  'e',  'r',  ' ', 
    'P',  'l',  'u',  'g',  'i',  'n',  0x77,  'O', 
    'p',  'e',  'n',  'R',  'G',  'B',  'P',  'l', 
    'u',  'g',  'i',  'n',  'A',  'P',  'I',  'V', 
    'e',  'r',  's',  'i',  'o',  'n',  0x05,  0x63, 
    'U',  'r',  'l',  0x60,  0x6a,  'V',  'e',  'r', 
    's',  'i',  'o',  'n',  'S',  't',  'r',  0x65, 
    '2',  '.',  '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN_V2(OpenGK850WPluginV2, OpenGK850WPluginV2, qt_pluginMetaDataV2_OpenGK850WPluginV2)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_OpenGK850WPluginV2[] = {
    'Q', 'T', 'M', 'E', 'T', 'A', 'D', 'A', 'T', 'A', ' ', '!',
    // metadata version, Qt version, architectural requirements
    0, QT_VERSION_MAJOR, QT_VERSION_MINOR, qPluginArchRequirements(),
    0xbf, 
    // "IID"
    0x02,  0x78,  0x22,  'o',  'r',  'g',  '.',  'o', 
    'p',  'e',  'n',  'r',  'g',  'b',  '.',  'O', 
    'p',  'e',  'n',  'R',  'G',  'B',  'P',  'l', 
    'u',  'g',  'i',  'n',  'I',  'n',  't',  'e', 
    'r',  'f',  'a',  'c',  'e', 
    // "className"
    0x03,  0x72,  'O',  'p',  'e',  'n',  'G',  'K', 
    '8',  '5',  '0',  'W',  'P',  'l',  'u',  'g', 
    'i',  'n',  'V',  '2', 
    // "MetaData"
    0x04,  0xa7,  0x66,  'C',  'o',  'm',  'm',  'i', 
    't',  0x60,  0x6b,  'D',  'e',  's',  'c',  'r', 
    'i',  'p',  't',  'i',  'o',  'n',  0x78,  0x49, 
    'V',  'i',  'r',  't',  'u',  'a',  'l',  ' ', 
    'c',  'o',  'n',  't',  'r',  'o',  'l',  'l', 
    'e',  'r',  ' ',  'p',  'l',  'u',  'g',  'i', 
    'n',  ' ',  'f',  'o',  'r',  ' ',  'G',  'K', 
    '8',  '5',  '0',  'W',  ' ',  'k',  'e',  'y', 
    'b',  'o',  'a',  'r',  'd',  ' ',  'w',  'i', 
    't',  'h',  ' ',  's',  'o',  'u',  'n',  'd', 
    ' ',  'r',  'e',  'a',  'c',  't',  'i',  'v', 
    'e',  ' ',  'e',  'f',  'f',  'e',  'c',  't', 
    's',  0x62,  'I',  'd',  0x72,  'O',  'p',  'e', 
    'n',  'G',  'K',  '8',  '5',  '0',  'W',  'P', 
    'l',  'u',  'g',  'i',  'n',  'V',  '2',  0x64, 
    'N',  'a',  'm',  'e',  0x78,  0x20,  'G',  'K', 
    '8',  '5',  '0',  'W',  ' ',  'V',  'i',  'r', 
    't',  'u',  'a',  'l',  ' ',  'C',  'o',  'n', 
    't',  'r',  'o',  'l',  'l',  'e',  'r',  ' ', 
    'P',  'l',  'u',  'g',  'i',  'n',  0x77,  'O', 
    'p',  'e',  'n',  'R',  'G',  'B',  'P',  'l', 
    'u',  'g',  'i',  'n',  'A',  'P',  'I',  'V', 
    'e',  'r',  's',  'i',  'o',  'n',  0x05,  0x63, 
    'U',  'r',  'l',  0x60,  0x6a,  'V',  'e',  'r', 
    's',  'i',  'o',  'n',  'S',  't',  'r',  0x65, 
    '2',  '.',  '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(OpenGK850WPluginV2, OpenGK850WPluginV2)
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
