
// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the ceusbkwrapperdrv_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// ceusbkwrapperdrv_API functions as being imported from a DLL, wheras this DLL sees symbols
// defined with this macro as being exported.
#ifdef ceusbkwrapperdrv_EXPORTS
#define ceusbkwrapperdrv_API __declspec(dllexport)
#else
#define ceusbkwrapperdrv_API __declspec(dllimport)
#endif

// This class is exported from the ceusbkwrapperdrv.dll
class ceusbkwrapperdrv_API Cceusbkwrapperdrv {
public:
    Cceusbkwrapperdrv(void);
    // TODO: add your methods here.
};

extern ceusbkwrapperdrv_API int nceusbkwrapperdrv;

ceusbkwrapperdrv_API int fnceusbkwrapperdrv(void);

