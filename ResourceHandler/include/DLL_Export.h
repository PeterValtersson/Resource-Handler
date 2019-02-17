#ifndef _DLL_EXPORT_RH_H_
#define _DLL_EXPORT_RH_H_
#ifdef _RH_EXPORT_
#define DECLSPEC_RH __declspec(dllexport)
#else
#define DECLSPEC_RH __declspec(dllimport)
#endif
#endif