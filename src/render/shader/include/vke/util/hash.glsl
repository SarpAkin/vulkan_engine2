#ifndef VKE_HASH_GLSL
#define VKE_HASH_GLSL

uint hash(uint x) {
    x ^= x >> 17;
    x *= 0xed5ad4bbU;
    x ^= x >> 11;
    x *= 0xac4c1b51U;
    x ^= x >> 15;
    x *= 0x31848babU;
    x ^= x >> 14;
    return x;
}

vec3 uint2vec3(uint i) {
    return vec3(
        float((i >> 00) & 0x3FF) / 1023.0,
        float((i >> 10) & 0x7FF) / 2047.0,
        float((i >> 21) & 0x7FF) / 2047.0 //
    );
}

#endif