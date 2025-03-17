#ifndef QUAT_UTIL_H
#define QUAT_UTIL_H

// translated from glm::mat3_cast
mat3 mat3_cast(in vec4 q) {
    mat3 result = mat3(1);
    float qxx   = q.x * q.x;
    float qyy   = q.y * q.y;
    float qzz   = q.z * q.z;
    float qxz   = q.x * q.z;
    float qxy   = q.x * q.y;
    float qyz   = q.y * q.z;
    float qwx   = q.w * q.x;
    float qwy   = q.w * q.y;
    float qwz   = q.w * q.z;

    result[0][0] = 1 - 2 * (qyy + qzz);
    result[0][1] = 2 * (qxy + qwz);
    result[0][2] = 2 * (qxz - qwy);

    result[1][0] = 2 * (qxy - qwz);
    result[1][1] = 1 - 2 * (qxx + qzz);
    result[1][2] = 2 * (qyz + qwx);

    result[2][0] = 2 * (qxz + qwy);
    result[2][1] = 2 * (qyz - qwx);
    result[2][2] = 1 - 2 * (qxx + qyy);
    return result;
}

//translated from glm::quat * glm::vec3
vec3 quat_rotate(in vec4 q, in vec3 v) {
    vec3 uv  = cross(q.xyz, v);
    vec3 uuv = cross(q.xyz, uv);

    return v + ((uv * q.w) + uuv) * 2;
}

#endif