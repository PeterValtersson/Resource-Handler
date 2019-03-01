#ifndef _DLL_EXPORT_RH_H_
#define _DLL_EXPORT_RH_H_
#ifdef _RH_EXPORT_
#define DECLSPEC_RH __declspec(dllexport)
#else
#define DECLSPEC_RH __declspec(dllimport)
#endif
#endif

#ifndef _DLL_EXPORT_RA_H_
#define _DLL_EXPORT_RA_H_
#ifdef _RA_EXPORT_
#define DECLSPEC_RA __declspec(dllexport)
#else
#define DECLSPEC_RA __declspec(dllimport)
#endif
#endif