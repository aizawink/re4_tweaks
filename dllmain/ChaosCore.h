#pragma once
#include "ChaosMath.h"

namespace aiza
{
inline float Bound(float value, float low, float high, float fallback)
{
    return std::isfinite(value) ? std::clamp(value, low, high) : fallback;
}
inline bool IsChaosTarget(unsigned id, bool heavy)
{
    // Exclude IDs with known Merchant aliases and all player/sub-character IDs.
    switch (id)
    {
    case 0x11: case 0x12: case 0x15: case 0x16: case 0x17:
    case 0x19: case 0x1A: case 0x1F: return true;
    case 0x2B: return heavy; // El Gigante; opt-in, no other bosses.
    default: return false;
    }
}
inline bool IsChaosGun(unsigned item)
{
    // SDK item IDs; skip eggs, knives, grenades, harpoons and launchers.
    switch (item)
    {
    case 3: case 33: case 35: case 37: case 39: case 41: case 42:
    case 44: case 45: case 46: case 47: case 48: case 52: case 55:
    case 64: case 71: case 81: case 83: case 148: return true;
    default: return false;
    }
}
struct Point { float x=0, y=0, z=0; };
inline bool Finite(Point p)
{
    return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z);
}
inline float Distance2(Point a, Point b)
{
    const float x=a.x-b.x, y=a.y-b.y, z=a.z-b.z;
    return x*x+y*y+z*z;
}
struct Flight
{
    Point origin, position;
    float age=0, duration=3, height=6000, spread=1200, dx=1, dz=0;
    int mode=0;
    bool active=false;
    void Start(Point from, Point player, int style, float lift, float seconds, float range)
    {
        origin=position=from; age=0;
        height=Bound(lift, 200, 30000, 6000);
        duration=Bound(seconds, 0.5f, 8, 3);
        spread=Bound(range, 0, 4000, 1200);
        mode=std::clamp(style, 0, 2);
        const float x=from.x-player.x, z=from.z-player.z;
        const float distance=std::sqrt(x*x+z*z);
        dx=distance>0.01f ? x/distance : 1;
        dz=distance>0.01f ? z/distance : 0;
        active=Finite(from) && Finite(player);
    }
    Point Step(float seconds)
    {
        if (!active) return position;
        age+=Bound(seconds, 0, 0.1f, 0);
        const float t=std::min(age/duration, 1.0f);
        if (t>=1)
        {
            active=false; position=origin; return position;
        }
        const float envelope=std::sin(3.14159265f*t);
        position=origin;
        position.y+=4*height*t*(1-t);
        if (mode==1) // outward arc, then back to the known launch origin
        {
            position.x+=dx*spread*envelope;
            position.z+=dz*spread*envelope;
        }
        if (mode==2) // orbit around the launch origin; taper in and out
        {
            const float angle=6*3.14159265f*t;
            position.x+=std::cos(angle)*spread*envelope;
            position.z+=std::sin(angle)*spread*envelope;
        }
        return position;
    }
    Point Cancel() { active=false; position=origin; return position; }
};
}
