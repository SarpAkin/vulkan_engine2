#ifndef VKE_VORONOI_NOISE_GLSL
#define VKE_VORONOI_NOISE_GLSL

#include "../hash.glsl"

vec2 voronoi_noise(vec3 pos, uint seed) {
    uvec3 cell = uvec3(ivec3(floor(pos)));
    vec3 f     = fract(pos);

    float closest_dist = 1E10;
    uint closest_hash  = 0;

    for (int x = -1; x <= 1; x++) {
        uint hash_x = hash((cell.x + x) ^ seed) ^ seed;
        for (int y = -1; y <= 1; y++) {
            uint hash_xy = hash((cell.y + y) ^ hash_x);
            for (int z = -1; z <= 1; z++) {
                uint hash_xyz = hash((cell.z + z) ^ hash_xy);

                vec3 cell_center = uint2vec3(hash_xyz) + vec3(x, y, z);

                vec3 dist_v = cell_center - f;
                float dist2 = dot(dist_v, dist_v);
                if (dist2 < closest_dist) {
                    closest_dist = dist2;
                    closest_hash = hash_xyz;
                }
            }
        }
    }

    return vec2(float(hash(closest_hash) & 0xFFFF) / float(0xFFFF), closest_dist);
}

#endif