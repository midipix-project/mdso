#ifndef MDSO_API_H
#define MDSO_API_H

#include <limits.h>

/* mdso_export */
#if	defined(__dllexport)
#define mdso_export __dllexport
#else
#define mdso_export
#endif

/* mdso_import */
#if	defined(__dllimport)
#define mdso_import __dllimport
#else
#define mdso_import
#endif

/* mdso_api */
#ifndef MDSO_APP
#if     defined (MDSO_EXPORT)
#define mdso_api mdso_export
#elif   defined (MDSO_IMPORT)
#define mdso_api mdso_import
#elif   defined (MDSO_STATIC)
#define mdso_api
#else
#define mdso_api
#endif
#else
#define mdso_api
#endif

#endif
