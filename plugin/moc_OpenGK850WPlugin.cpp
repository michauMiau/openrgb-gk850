/****************************************************************************
** Meta object code from reading C++ file 'OpenGK850WPlugin.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.8.2)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include "OpenGK850WPlugin.h"
#include <QtGui/qtextcursor.h>
#include <QtCore/qmetatype.h>
#include <QtCore/qplugin.h>

#include <QtCore/qtmochelpers.h>

#include <memory>


#include <QtCore/qxptype_traits.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'OpenGK850WPlugin.h' doesn't include <QObject>."
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
struct qt_meta_tag_ZN16OpenGK850WPluginE_t {};
} // unnamed namespace


#ifdef QT_MOC_HAS_STRINGDATA
static constexpr auto qt_meta_stringdata_ZN16OpenGK850WPluginE = QtMocHelpers::stringData(
    "OpenGK850WPlugin"
);
#else  // !QT_MOC_HAS_STRINGDATA
#error "qtmochelpers.h not found or too old."
#endif // !QT_MOC_HAS_STRINGDATA

Q_CONSTINIT static const uint qt_meta_data_ZN16OpenGK850WPluginE[] = {

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

Q_CONSTINIT const QMetaObject OpenGK850WPlugin::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_ZN16OpenGK850WPluginE.offsetsAndSizes,
    qt_meta_data_ZN16OpenGK850WPluginE,
    qt_static_metacall,
    nullptr,
    qt_incomplete_metaTypeArray<qt_meta_tag_ZN16OpenGK850WPluginE_t,
        // Q_OBJECT / Q_GADGET
        QtPrivate::TypeAndForceComplete<OpenGK850WPlugin, std::true_type>
    >,
    nullptr
} };

void OpenGK850WPlugin::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    auto *_t = static_cast<OpenGK850WPlugin *>(_o);
    (void)_t;
    (void)_c;
    (void)_id;
    (void)_a;
}

const QMetaObject *OpenGK850WPlugin::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *OpenGK850WPlugin::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_ZN16OpenGK850WPluginE.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "OpenRGBPluginInterface"))
        return static_cast< OpenRGBPluginInterface*>(this);
    if (!strcmp(_clname, "org.openrgb.OpenRGBPluginInterface"))
        return static_cast< OpenRGBPluginInterface*>(this);
    return QObject::qt_metacast(_clname);
}

int OpenGK850WPlugin::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    return _id;
}

#ifdef QT_MOC_EXPORT_PLUGIN_V2
static constexpr unsigned char qt_pluginMetaDataV2_OpenGK850WPlugin[] = {
    0xbf, 
    // "IID"
    0x02,  0x78,  0x22,  'o',  'r',  'g',  '.',  'o', 
    'p',  'e',  'n',  'r',  'g',  'b',  '.',  'O', 
    'p',  'e',  'n',  'R',  'G',  'B',  'P',  'l', 
    'u',  'g',  'i',  'n',  'I',  'n',  't',  'e', 
    'r',  'f',  'a',  'c',  'e', 
    // "className"
    0x03,  0x70,  'O',  'p',  'e',  'n',  'G',  'K', 
    '8',  '5',  '0',  'W',  'P',  'l',  'u',  'g', 
    'i',  'n', 
    // "MetaData"
    0x04,  0xa7,  0x66,  'C',  'o',  'm',  'm',  'i', 
    't',  0x60,  0x6b,  'D',  'e',  's',  'c',  'r', 
    'i',  'p',  't',  'i',  'o',  'n',  0x78,  0x3d, 
    'P',  'l',  'u',  'g',  'i',  'n',  ' ',  'i', 
    'm',  'p',  'l',  'e',  'm',  'e',  'n',  't', 
    'i',  'n',  'g',  ' ',  's',  'u',  'p',  'p', 
    'o',  'r',  't',  ' ',  'f',  'o',  'r',  ' ', 
    't',  'h',  'e',  ' ',  'B',  'Y',  ' ',  'T', 
    'e',  'c',  'h',  '/',  'M',  'a',  'd',  ' ', 
    'D',  'o',  'g',  ' ',  'G',  'K',  '8',  '5', 
    '0',  '(',  'W',  ')',  ' ',  0x62,  'I',  'd', 
    0x70,  'O',  'p',  'e',  'n',  'G',  'K',  '8', 
    '5',  '0',  'W',  'P',  'l',  'u',  'g',  'i', 
    'n',  0x64,  'N',  'a',  'm',  'e',  0x78,  0x20, 
    'G',  'K',  '8',  '5',  '0',  'W',  ' ',  'V', 
    'i',  'r',  't',  'u',  'a',  'l',  ' ',  'C', 
    'o',  'n',  't',  'r',  'o',  'l',  'l',  'e', 
    'r',  ' ',  'P',  'l',  'u',  'g',  'i',  'n', 
    0x77,  'O',  'p',  'e',  'n',  'R',  'G',  'B', 
    'P',  'l',  'u',  'g',  'i',  'n',  'A',  'P', 
    'I',  'V',  'e',  'r',  's',  'i',  'o',  'n', 
    0x05,  0x63,  'U',  'r',  'l',  0x78,  0x2b,  'h', 
    't',  't',  'p',  's',  ':',  '/',  '/',  'g', 
    'i',  't',  'h',  'u',  'b',  '.',  'c',  'o', 
    'm',  '/',  'm',  'i',  'c',  'h',  'a',  'u', 
    'M',  'i',  'a',  'u',  '/',  'o',  'p',  'e', 
    'n',  'r',  'g',  'b',  '-',  'g',  'k',  '8', 
    '5',  '0',  0x6a,  'V',  'e',  'r',  's',  'i', 
    'o',  'n',  'S',  't',  'r',  0x65,  '2',  '.', 
    '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN_V2(OpenGK850WPlugin, OpenGK850WPlugin, qt_pluginMetaDataV2_OpenGK850WPlugin)
#else
QT_PLUGIN_METADATA_SECTION
Q_CONSTINIT static constexpr unsigned char qt_pluginMetaData_OpenGK850WPlugin[] = {
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
    0x03,  0x70,  'O',  'p',  'e',  'n',  'G',  'K', 
    '8',  '5',  '0',  'W',  'P',  'l',  'u',  'g', 
    'i',  'n', 
    // "MetaData"
    0x04,  0xa7,  0x66,  'C',  'o',  'm',  'm',  'i', 
    't',  0x60,  0x6b,  'D',  'e',  's',  'c',  'r', 
    'i',  'p',  't',  'i',  'o',  'n',  0x78,  0x3d, 
    'P',  'l',  'u',  'g',  'i',  'n',  ' ',  'i', 
    'm',  'p',  'l',  'e',  'm',  'e',  'n',  't', 
    'i',  'n',  'g',  ' ',  's',  'u',  'p',  'p', 
    'o',  'r',  't',  ' ',  'f',  'o',  'r',  ' ', 
    't',  'h',  'e',  ' ',  'B',  'Y',  ' ',  'T', 
    'e',  'c',  'h',  '/',  'M',  'a',  'd',  ' ', 
    'D',  'o',  'g',  ' ',  'G',  'K',  '8',  '5', 
    '0',  '(',  'W',  ')',  ' ',  0x62,  'I',  'd', 
    0x70,  'O',  'p',  'e',  'n',  'G',  'K',  '8', 
    '5',  '0',  'W',  'P',  'l',  'u',  'g',  'i', 
    'n',  0x64,  'N',  'a',  'm',  'e',  0x78,  0x20, 
    'G',  'K',  '8',  '5',  '0',  'W',  ' ',  'V', 
    'i',  'r',  't',  'u',  'a',  'l',  ' ',  'C', 
    'o',  'n',  't',  'r',  'o',  'l',  'l',  'e', 
    'r',  ' ',  'P',  'l',  'u',  'g',  'i',  'n', 
    0x77,  'O',  'p',  'e',  'n',  'R',  'G',  'B', 
    'P',  'l',  'u',  'g',  'i',  'n',  'A',  'P', 
    'I',  'V',  'e',  'r',  's',  'i',  'o',  'n', 
    0x05,  0x63,  'U',  'r',  'l',  0x78,  0x2b,  'h', 
    't',  't',  'p',  's',  ':',  '/',  '/',  'g', 
    'i',  't',  'h',  'u',  'b',  '.',  'c',  'o', 
    'm',  '/',  'm',  'i',  'c',  'h',  'a',  'u', 
    'M',  'i',  'a',  'u',  '/',  'o',  'p',  'e', 
    'n',  'r',  'g',  'b',  '-',  'g',  'k',  '8', 
    '5',  '0',  0x6a,  'V',  'e',  'r',  's',  'i', 
    'o',  'n',  'S',  't',  'r',  0x65,  '2',  '.', 
    '0',  '.',  '0', 
    0xff, 
};
QT_MOC_EXPORT_PLUGIN(OpenGK850WPlugin, OpenGK850WPlugin)
#endif  // QT_MOC_EXPORT_PLUGIN_V2

QT_WARNING_POP
