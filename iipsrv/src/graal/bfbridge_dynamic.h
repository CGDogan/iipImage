#ifndef __BFBRIDGE_H
#define __BFBRIDGE_H

#include <graal_isolate_dynamic.h>


#if defined(__cplusplus)
extern "C" {
#endif

typedef char* (*BFBridge__BFGetError__f7fbc905a0bd7a649aa7f3df70ee642476cab49f_fn_t)(graal_isolatethread_t*);

typedef char (*BFBridge__BFOpen__d4979d204df1fb7e54908c4c2b050bef8d67d588_fn_t)(graal_isolatethread_t*, char*);

typedef char (*BFBridge__BFIIsSingleFile__896cd9ac8173f21c66697b825d432e9ae251af67_fn_t)(graal_isolatethread_t*, char*);

typedef char (*BFBridge__BFClose__36267f1bb00c360d9c78f6fa6a947fd06895ea8c_fn_t)(graal_isolatethread_t*, int);

typedef int (*BFBridge__BFGetResolutionCount__faa1c8e4696f774d6c806495a514563dd387c4a2_fn_t)(graal_isolatethread_t*);

typedef char (*BFBridge__BFSetCurrentResolution__552c22071d7df69e94f05c9384b0d9dd6099671c_fn_t)(graal_isolatethread_t*, int);

typedef int (*BFBridge__BFGetSizeX__c2f0fc9b4dc26bb9b2e7b237d447e22d63449693_fn_t)(graal_isolatethread_t*);

typedef int (*BFBridge__BFGetSizeY__749eb03f7ddd4431619a0496d42c3b05751790a5_fn_t)(graal_isolatethread_t*);

typedef int (*BFBridge__BFGetOptimalTileWidth__bf8e8d9a96ff1d73382fc73f60304bbfcf79979f_fn_t)(graal_isolatethread_t*);

typedef int (*BFBridge__BFGetOptimalT_0130leHeight__06f1fcc33db1dd933d002ac4a0b6fb00b588a05a_fn_t)(graal_isolatethread_t*);

typedef char* (*BFBridge__BFGetFormat__825dce3e39ba1ccc8f2b5b4afd4e51d8edf496bf_fn_t)(graal_isolatethread_t*);

typedef int (*BFBridge__BFGetPixelType__270026b6812a0d40486767490a241181cd6b8412_fn_t)(graal_isolatethread_t*);

typedef int (*BFBridge__BFGetBitsPerPixel__3ff7321e85d6460749a043722d7db8ec1a86f779_fn_t)(graal_isolatethread_t*);

typedef int (*BFBridge__BFGetBytesPerPixel__27d851624c1a806d40411932cd2e44a119d049c1_fn_t)(graal_isolatethread_t*);

typedef int (*BFBridge__BFGetRGBChannelCount__c695446d8c2580cb80c0f543c0a2c2681c8fef5c_fn_t)(graal_isolatethread_t*);

typedef char (*BFBridge__BFIsRGB__bbffb6d873f57b88ad64fe5571a5914caaefcbc8_fn_t)(graal_isolatethread_t*);

typedef char (*BFBridge__BFIsInterleaved__f98ab628b3d84556dd918d14659a277b878cf4c7_fn_t)(graal_isolatethread_t*);

typedef char (*BFBridge__BFIsLittleEndian__8a78bd79fb7c6ed65de1e9cf92635d48d2deae24_fn_t)(graal_isolatethread_t*);

typedef char (*BFBridge__BFIsFloatingPoint__3495cf35e009588f31a30f1993e02cf7b353cc24_fn_t)(graal_isolatethread_t*);

typedef char* (*BFBridge__BFIGetDimensionOrder__cec400d17f357268fff6ea76ff6beed1d6ebe333_fn_t)(graal_isolatethread_t*);

typedef char* (*BFBridge__BFIOpenBytes__5ad7e1c67d31f1c26837fac6abe358a4855caf38_fn_t)(graal_isolatethread_t*, int, int, int, int);

typedef int (*run_main_fn_t)(int argc, char** argv);

#if defined(__cplusplus)
}
#endif
#endif
