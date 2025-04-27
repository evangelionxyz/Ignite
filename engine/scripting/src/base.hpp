#pragma once

#ifdef IGNITE_EXPORT_DLL
#define IGNITE_API __declspec(dllexport)
#else
#define IGNITE_API //__declspec(dllimport)
#endif
