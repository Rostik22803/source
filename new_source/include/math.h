#ifndef NEW_SOURCE_MATH_H
#define NEW_SOURCE_MATH_H

#include <cmath>
#include <cstdint>

struct Vec3 {
    float x = 0, y = 0, z = 0;
    Vec3() {}
    Vec3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    Vec3 operator-(const Vec3 &o) const { return Vec3(x - o.x, y - o.y, z - o.z); }
    Vec3 operator+(const Vec3 &o) const { return Vec3(x + o.x, y + o.y, z + o.z); }
    Vec3 operator*(float s) const { return Vec3(x * s, y * s, z * s); }
    float length() const { return sqrtf(x * x + y * y + z * z); }
    float dot(const Vec3 &o) const { return x * o.x + y * o.y + z * o.z; }
    float dist(const Vec3 &o) const { return (*this - o).length(); }
    bool zero() const { return x == 0.f && y == 0.f && z == 0.f; }
};

struct Vec2 { float x = 0, y = 0; };

struct Quat { float x = 0, y = 0, z = 0, w = 1; };

// Мировой кватернион камеры -> yaw/pitch в градусах (конвенция игры:
// pitch вниз +, yaw вокруг Y). forward = (sy*cp, -sp, cy*cp):
//   forward.y = -sin(pitch)  -> pitch = -asin(forward.y)
//   forward.x/z от yaw.
inline void quat_to_yaw_pitch(const Quat &q, float &yaw, float &pitch) {
    // forward = rotate(q, (0,0,1)), Unity-конвенция:
    //   x = 2(x z + w y), y = 2(y z - w x), z = 1 - 2(x^2 + y^2)
    float fx = 2.f * (q.x * q.z + q.w * q.y);
    float fy = 2.f * (q.y * q.z - q.w * q.x);
    float fz = 1.f - 2.f * (q.x * q.x + q.y * q.y);
    // В Unity поворот камеры вниз даётся положительным вращением вокруг +X;
    // forward при этом смотрит вниз (fy<0), а MouseLook.pitch вниз = "+".
    pitch = asinf(fy < -1.f ? -1.f : (fy > 1.f ? 1.f : fy)) * 57.2957795f;
    yaw   = atan2f(fx, fz) * 57.2957795f;
}

// Камера. Углы в ГРАДУСАХ, конвенция как у игры (MouseLook 0x60/0x64):
//   pitch ПОЛОЖИТЕЛЬНЫЙ = смотрим ВНИЗ (вверх — отрицательный),
//   yaw — вокруг оси Y, forward при yaw=0 смотрит в +Z.
struct ViewBasis {
    Vec3  pos;
    float yaw = 0.f;
    float pitch = 0.f;
    float vfov = 60.f;  // вертикальный FOV

    Vec3 forward() const {
        float ry = yaw   * 0.01745329f;
        float rp = pitch * 0.01745329f;
        // pitch вниз + => world Y компонента = -sin(pitch)
        float cp = cosf(rp);
        return Vec3(sinf(ry) * cp, -sinf(rp), cosf(ry) * cp);
    }
};

// World-to-screen. false если точка за камерой.
inline bool world_to_screen(const ViewBasis &cam, const Vec3 &world,
                            float screenW, float screenH, Vec2 &out) {
    Vec3 d = world - cam.pos;
    float ry = -cam.yaw   * 0.01745329f;
    float rp = -cam.pitch * 0.01745329f;

    // yaw вокруг Y
    float cy = cosf(ry), sy = sinf(ry);
    Vec3 d1(d.x * cy + d.z * sy,
            d.y,
           -d.x * sy + d.z * cy);
    // pitch вокруг X (pitch вниз +, поэтому инвертируем знак через rp)
    float cx = cosf(rp), sx = sinf(rp);
    Vec3 d2(d1.x,
            d1.y * cx + d1.z * sx,
           -d1.y * sx + d1.z * cx);

    if (d2.z < 0.1f) return false; // позади камеры

    float tanV   = tanf(cam.vfov * 0.5f * 0.01745329f);
    float aspect = screenW / screenH;
    float nx = d2.x / (d2.z * tanV * aspect);
    float ny = d2.y / (d2.z * tanV);

    out.x = (screenW * 0.5f) + (nx * screenW * 0.5f);
    out.y = (screenH * 0.5f) - (ny * screenH * 0.5f);
    return true;
}

// Углы на цель из точки eye (та же конвенция, что пишет аимбот в 0x60/0x64).
inline void angles_to_target(const Vec3 &eye, const Vec3 &target,
                             float &outPitch, float &outYaw) {
    Vec3 d = target - eye;
    float horiz = sqrtf(d.x * d.x + d.z * d.z);
    outPitch = -atan2f(d.y, horiz) * 57.2957795f; // цель выше => pitch отрицательный
    outYaw   =  atan2f(d.x, d.z)   * 57.2957795f;
}

inline float normalize_angle(float a) {
    while (a > 180.f)  a -= 360.f;
    while (a < -180.f) a += 360.f;
    return a;
}

#endif // NEW_SOURCE_MATH_H
