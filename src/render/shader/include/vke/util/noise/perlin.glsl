#ifndef VKE_PERLIN_NOISE_GLSL
#define VKE_PERLIN_NOISE_GLSL

#include "../hash.glsl"

float fade572(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}
float d_fade572(float t) {
    return t * t * (t * (30 * t - 60) + 30);
}
vec3 fade572(vec3 t) {
    return vec3(fade572(t.x), fade572(t.y), fade572(t.z));
}
vec3 d_fade572(vec3 t) {
    return vec3(d_fade572(t.x), d_fade572(t.y), d_fade572(t.z));
}

vec3 grad_vec572(uint hash) {
    vec3 table[] = {
            vec3(01, 01, 00),
            vec3(-1, 01, 00),
            vec3(01, -1, 00),
            vec3(-1, -1, 00),

            vec3(01, 00, 01),
            vec3(-1, 00, 01),
            vec3(01, 00, -1),
            vec3(-1, 00, -1),

            vec3(00, 01, 01),
            vec3(00, -1, 01),
            vec3(00, 01, -1),
            vec3(00, -1, -1),

            vec3(01, 01, 00), // 0 u v
            vec3(00, -1, 01), // v u 0
            vec3(-1, 01, 00), // 0 u v
            vec3(00, -1, -1), // v u 0
        };

    return table[hash & 15];
}

float grad572(uint hash, float x, float y, float z) {
    return dot(vec3(x, y, z), grad_vec572(hash));
    uint h = hash & 15; // CONVERT LO 4 BITS OF HASH CODE
    float u = h < 8 ? x : y; // INTO 12 GRADIENT DIRECTIONS.
    float v = h < 4 ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float perlin_noise(vec3 p, uint seed) {
    uvec3 I = uvec3(ivec3(floor(p)));
    vec3 F = fract(p);
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

vec4 perlin_noise_gradient2(vec3 p, uint seed) {
    const float h = 0.001;
    const float ih = 1000;

    float v = perlin_noise(p, seed);
    float x = perlin_noise(p + vec3(h, 0, 0), seed);
    float y = perlin_noise(p + vec3(0, h, 0), seed);
    float z = perlin_noise(p + vec3(0, 0, h), seed);

    return vec4(v, (x - v) * ih, (y - v) * ih, (z - v) * ih);
}

vec4 perlin_noise_gradient(vec3 p, uint seed) {
    uvec3 I = uvec3(ivec3(floor(p)));
    vec3 F = fract(p);
    vec3 faded = fade572(F);
    vec3 d_faded = d_fade572(F);
    float x = F.x, y = F.y, z = F.z;

    uint A = (hash((I.x + 0) ^ seed) ^ seed) + I.y, AA = hash(A) + I.z, AB = hash(A + 1) + I.z;
    uint B = (hash((I.x + 1) ^ seed) ^ seed) + I.y, BA = hash(B) + I.z, BB = hash(B + 1) + I.z;

    // x y z
    float dot000 = grad572(hash(AA + 0), x, y - 0, z - 0), dot100 = grad572(hash(BA + 0), x - 1, y - 0, z - 0);
    float dot010 = grad572(hash(AB + 0), x, y - 1, z - 0), dot110 = grad572(hash(BB + 0), x - 1, y - 1, z - 0);

    float dot001 = grad572(hash(AA + 1), x, y - 0, z - 1), dot101 = grad572(hash(BA + 1), x - 1, y - 0, z - 1);
    float dot011 = grad572(hash(AB + 1), x, y - 1, z - 1), dot111 = grad572(hash(BB + 1), x - 1, y - 1, z - 1);

    vec3 g000 = grad_vec572(hash(AA + 0)), g100 = grad_vec572(hash(BA + 0));
    vec3 g010 = grad_vec572(hash(AB + 0)), g110 = grad_vec572(hash(BB + 0));

    vec3 g001 = grad_vec572(hash(AA + 1)), g101 = grad_vec572(hash(BA + 1));
    vec3 g011 = grad_vec572(hash(AB + 1)), g111 = grad_vec572(hash(BB + 1));

    float u = faded.x, v = faded.y, w = faded.z;
    float uv = u * v, uw = u * w, vw = v * w;
    float uvw = uv * w;

    float du = d_faded.x, dv = d_faded.y, dw = d_faded.z;

    float n = dot000;
    n += u * (dot100 - dot000);
    n += v * (dot010 - dot000);
    n += w * (dot001 - dot000);
    n += uv * (dot110 - dot010 - dot100 + dot000);
    n += uw * (dot101 - dot001 - dot100 + dot000);
    n += vw * (dot011 - dot001 - dot010 + dot000);
    n += uvw * (dot111 - dot011 - dot101 + dot001 - dot110 + dot010 + dot100 - dot000);

    float dx = g000.x;
    dx += u * (g100.x - g000.x);
    dx += v * (g010.x - g000.x);
    dx += w * (g001.x - g000.x);
    dx += uv * (g110.x - g010.x - g100.x + g000.x);
    dx += uw * (g101.x - g001.x - g100.x + g000.x);
    dx += vw * (g011.x - g001.x - g010.x + g000.x);
    dx += uvw * (g111.x - g011.x - g101.x + g001.x - g110.x + g010.x + g100.x - g000.x);

    dx += du * (dot100 - dot000);
    dx += du * v * (dot110 - dot010 - dot100 + dot000);
    dx += du * w * (dot101 - dot001 - dot100 + dot000);
    dx += du * vw * (dot111 - dot011 - dot101 + dot001 - dot110 + dot010 + dot100 - dot000);

    float dy = g000.y;
    dy += u * (g100.y - g000.y);
    dy += v * (g010.y - g000.y);
    dy += w * (g001.y - g000.y);
    dy += uv * (g110.y - g010.y - g100.y + g000.y);
    dy += uw * (g101.y - g001.y - g100.y + g000.y);
    dy += vw * (g011.y - g001.y - g010.y + g000.y);
    dy += uvw * (g111.y - g011.y - g101.y + g001.y - g110.y + g010.y + g100.y - g000.y);

    dy += dv * (dot010 - dot000);
    dy += dv * u * (dot110 - dot010 - dot100 + dot000);
    dy += dv * w * (dot011 - dot001 - dot010 + dot000);
    dy += dv * uw * (dot111 - dot011 - dot101 + dot001 - dot110 + dot010 + dot100 - dot000);

    float dz = g000.z;
    dz += u * (g100.z - g000.z);
    dz += v * (g010.z - g000.z);
    dz += w * (g001.z - g000.z);
    dz += uv * (g110.z - g010.z - g100.z + g000.z);
    dz += uw * (g101.z - g001.z - g100.z + g000.z);
    dz += vw * (g011.z - g001.z - g010.z + g000.z);
    dz += uvw * (g111.z - g011.z - g101.z + g001.z - g110.z + g010.z + g100.z - g000.z);

    dz += dw * (dot001 - dot000);
    dz += dw * u * (dot101 - dot001 - dot100 + dot000);
    dz += dw * v * (dot011 - dot001 - dot010 + dot000);
    dz += dw * uv * (dot111 - dot011 - dot101 + dot001 - dot110 + dot010 + dot100 - dot000);

    return vec4(n, dx, dy, dz);
}

float layered_perlin_noise(vec3 p, uint seed, uint levels) {
    float total = 0.0;

    float freq = 1.0, amp = 0.5 /* we set amp to 0.5 to compensate for the added range*/ ;

    for (int i = 0; i < levels; i++) {
        total += perlin_noise(p * freq, seed + i);
        freq *= 2.0;
        amp *= 0.5;
    }

    // return total;/*enable this for a small optimization at the cost of subpar normalization for low levels */
    return total / (1.0 - amp);
}

vec4 layered_perlin_noise_gradient(vec3 p, uint seed, uint levels) {
    vec4 total = vec4(0.0);

    float freq = 1.0, amp = 0.5 /* we set amp to 0.5 to compensate for the added range*/ ;

    for (int i = 0; i < levels; i++) {
        total += perlin_noise_gradient(p * freq, seed + i);
        freq *= 2.0;
        amp *= 0.5;
    }

    // return total;/*enable this for a small optimization at the cost of subpar normalization for low levels */
    return total / (1.0 - amp);
}

#endif
