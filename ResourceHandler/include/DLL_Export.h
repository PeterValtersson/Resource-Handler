#ifndef _DLL_EXPORT_H_
#define _DLL_EXPORT_H_
#ifdef _RH_EXPORT_
#define DECLSPEC __declspec(dllexport)
#else
#define DECLSPEC __declspec(dllimport)
#endif
#endif