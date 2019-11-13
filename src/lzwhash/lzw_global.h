#ifndef LZW_GLOBAL_H
#define LZW_GLOBAL_H

#include <QtCore/qglobal.h>

#ifdef EXPORTING_LZW
#define LZW_API Q_DECL_EXPORT
#else
#define LZW_API Q_DECL_IMPORT
#endif

#endif // LZW_GLOBAL_H
