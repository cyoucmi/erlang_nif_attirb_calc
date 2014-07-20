#ifndef __header__h__

//#define ERL_DRV 1

#ifdef ERL_DRV

#include "erl_driver.h"
#define MALLOC driver_alloc
#define REALLOC driver_realloc
#define FREE driver_free

#elif defined(ERL_NIF)

#include "erl_nif.h"
#define MALLOC enif_alloc 
#define REALLOC enif_realloc
#define FREE enif_free

#else

#define MALLOC malloc
#define REALLOC realloc
#define FREE free

#endif

#ifdef ERL_PRINT
#define PRINTF(format,str) printf(format,str)

#else

#define PRINTF(format,str) 

#endif
#endif //__header__h__
