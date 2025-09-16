#ifndef VKE_FS_OUT_AUTO
#define VKE_FS_OUT_AUTO

#ifdef GPASS
#include "gpass.glsl"
#elif SHADOW_PASS
#include "shadow.glsl"
#else
#include "default.glsl"
#endif


#endif