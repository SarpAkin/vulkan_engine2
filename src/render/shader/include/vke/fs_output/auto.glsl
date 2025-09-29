#ifndef VKE_FS_OUT_AUTO
#define VKE_FS_OUT_AUTO

#if defined(GPASS)
#include "gpass.glsl"
#elif defined(SHADOW_PASS)
#include "shadow.glsl"
#else
#include "default.glsl"
#endif


#endif