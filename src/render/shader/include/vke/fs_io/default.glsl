#ifndef VKE_FS_IO_DEFAULT
#define VKE_FS_IO_DEFAULT

#ifndef VKE_FS_IO_MODE

#if defined(VERTEX_SHADER)
#define VKE_FS_IO_MODE out
#elif defined(TESS_EVALUATION_SHADER)
#define VKE_FS_IO_MODE out
#elif defined(FRAGMENT_SHADER)
#define VKE_FS_IO_MODE in
#else
#define VKE_FS_IO_MODE
#define VKE_FS_IO_MODE_EQ_NONE
#endif

#endif // end of VKE_FS_IO_MODE

#ifdef VKE_FS_IO_MODE_EQ_NONE
#define VKE_IO_LAYOUT(l, dec) dec
#else
#define VKE_IO_LAYOUT(l, dec) layout(location = l) VKE_FS_IO_MODE dec

#endif

VKE_IO_LAYOUT(0, vec3 f_color);
VKE_IO_LAYOUT(1, vec2 f_uvs);
VKE_IO_LAYOUT(2, vec3 f_normal);
VKE_IO_LAYOUT(3, vec3 f_position);

#define VKE_FS_IO_LAST 3

#undef VKE_IO_LAYOUT

#endif