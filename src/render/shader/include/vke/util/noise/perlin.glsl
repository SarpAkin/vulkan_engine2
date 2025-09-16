#ifndef VKE_PERLIN_NOISE_GLSL
#define VKE_PERLIN_NOISE_GLSL

#include "../hash.glsl"

float fade572(float t) { return t * t * t * (t * (t * 6 - 15) + 10); }
vec3 fade572(vec3 t) { return vec3(fade572(t.x), fade572(t.y), fade572(t.z)); }

float grad572(uint hash, float x, float y, float z) {
    uint h  = hash & 15;     // CONVERT LO 4 BITS OF HASH CODE
    float u = h < 8 ? x : y, // INTO 12 GRADIENT DIRECTIONS.
        v   = h < 4 ? y : h == 12 || h == 14 ? x
                                             : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlin_noise(vec3 p, uint seed) {
    uvec3 I    = uvec3(ivec3(floor(p)));
    vec3 F     = fract(p);
    vec3 faded = fade572(F);
    float x = F.x, y = F.y, z = F.z;

    uint A = (hash((I.x + 0) ^ seed) ^ seed) + I.y, AA = hash(A) + I.z, AB = hash(A + 1) + I.z;
    uint B = (hash((I.x + 1) ^ seed) ^ seed) + I.y, BA = hash(B) + I.z, BB = hash(B + 1) + I.z;

    float v =
        mix(mix(mix(grad572(hash(AA + 0), x, y - 0, z - 0), grad572(hash(BA + 0), x - 1, y - 0, z - 0), faded.x),
                mix(grad572(hash(AB + 0), x, y - 1, z - 0), grad572(hash(BB + 0), x - 1, y - 1, z - 0), faded.x), faded.y),
            mix(mix(grad572(hash(AA + 1), x, y - 0, z - 1), grad572(hash(BA + 1), x - 1, y - 0, z - 1), faded.x),
                mix(grad572(hash(AB + 1), x, y - 1, z - 1), grad572(hash(BB + 1), x - 1, y - 1, z - 1), faded.x), faded.y),
            faded.z);

    return v;
}

float layered_perlin_noise(vec3 p, uint seed, uint levels) {
    float total = 0.0;

    float freq = 1.0, amp = 0.5 /* we set amp to 0.5 to compensate for the added range*/;

    for (int i = 0; i < levels; i++) {
        total += perlin_noise(p * freq, seed + i);
        freq *= 2.0;
        amp *= 0.5;
    }

    // return total;/*enable this for a small optimization at the cost of subpar normalization for low levels */
    return total / (1.0 - amp);
}

#endif