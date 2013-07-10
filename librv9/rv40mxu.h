#ifndef RV40MXU_H
#define RV40MXU_H
#define MXU_SETZERO(addr,cline)   \
    do {                            \
       int32_t mxu_i;               \
       int32_t local = (int32_t)(addr)-4;  \
       for (mxu_i=0; mxu_i < cline; mxu_i++) \
       {                                     \
	   S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
           S32SDI(xr0,local,4);              \
       }                                     \
    }while(0)
#endif
