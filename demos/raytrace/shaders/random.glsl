
uint hash(uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float floatConstruct(uint m) {
    const uint ieeeMantissa = 0x007FFFFFu; // binary32 mantissa bitmask
    const uint ieeeOne      = 0x3F800000u; // 1.0 in IEEE binary32

    m &= ieeeMantissa; // Keep only mantissa bits (fractional part)
    m |= ieeeOne;      // Add fractional part to 1.0

    float f = uintBitsToFloat(m); // Range [1:2]
    return f - 1.0;               // Range [0:1]
}

uint random_state = 0;
float urandom() {
    random_state = hash(random_state);
    return floatConstruct(random_state);
}
float srandom() {
    return urandom() * 2.0 - 1;
}

vec3 srandom3() {
    return vec3(srandom(), srandom(), srandom());
}

void seed_random(vec2 vec) {
    uvec2 bits   = floatBitsToInt(vec);
    random_state = (bits.x + 0xAF02563D) ^ bits.y << 4 + bits.y;
}