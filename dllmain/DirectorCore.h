#pragma once
#include "ChaosCore.h"
#include <cstddef>
#include <cstdint>
#include <limits>

namespace aiza
{
inline uint32_t MixSeed(uint32_t seed, uint32_t value)
{
    uint32_t x=seed ^ (value+0x9e3779b9u+(seed<<6)+(seed>>2));
    x^=x>>16; x*=0x7feb352du; x^=x>>15; x*=0x846ca68bu; return x^(x>>16);
}
struct Random
{
    uint32_t state=1;
    explicit Random(uint32_t seed=1):state(seed ? seed : 1) {}
    uint32_t Next() { state^=state<<13;state^=state>>17;state^=state<<5;return state; }
    float Unit() { return float(Next()>>8)/16777216.0f; }
    unsigned Pick(unsigned count) { return count ? Next()%count : 0; }
};
inline uint64_t TemplateKey(unsigned id, unsigned type, unsigned character, uint32_t flags)
{
    // Deliberately omit position, room-event routine and HP from appearance identity.
    return uint64_t(id&255) | (uint64_t(type&255)<<8) | (uint64_t(character&255)<<16) |
        (uint64_t(flags & 0xffffff00u)<<24);
}
inline bool CatalogHumanoid(unsigned id)
{
    switch(id) {
    case 0x11:case 0x12:case 0x15:case 0x16:case 0x17:
    case 0x19:case 0x1a:case 0x1b:case 0x1c:case 0x1d:case 0x1f:case 0x20:return true;
    default:return false;
    }
}
inline bool ReplacementHumanoid(unsigned id)
{
    return CatalogHumanoid(id) && id!=0x1b && id!=0x1c && id!=0x1d;
}
inline bool CatalogTarget(unsigned id, bool heavy)
{
    return CatalogHumanoid(id) || (heavy && (id==0x22 || id==0x2b || id==0x2d || id==0x36 || id==0x3c));
}
inline bool ChainsawAppearance(unsigned id,unsigned type,uint32_t flags)
{
    // Cross-check against a native room template, never synthesize this record.
    return (id==0x12 || id==0x15 || id==0x16 || id==0x17) && type==4 && (flags>>24)==0x10;
}
inline bool EncodeESLPosition(Point point, int16_t& x,int16_t& y,int16_t& z)
{
    if (!Finite(point)) return false;
    const double a=std::round(double(point.x)/10), b=std::round(double(point.y)/10), c=std::round(double(point.z)/10);
    if (a<-32768 || a>32767 || b<-32768 || b>32767 || c<-32768 || c>32767) return false;
    x=int16_t(a);y=int16_t(b);z=int16_t(c);return true;
}
inline unsigned WaveCount(int start,int growth,int wave)
{
    const int64_t count=int64_t(start)+std::max<int64_t>(0,int64_t(wave)-1)*growth;
    return unsigned(std::clamp<int64_t>(count,1,20));
}
inline float WaveHPFactor(int wave,float growth)
{
    return Bound(1+float(std::max<int64_t>(0,int64_t(wave)-1))*Bound(growth,0,2,.25f),1,15,1);
}
inline bool FrontCone(Point origin, Point point,float yaw,float radius)
{
    const float x=point.x-origin.x,z=point.z-origin.z;
    const float length=std::sqrt(x*x+z*z);
    return length>1 && length<=radius && std::abs(point.y-origin.y)<900 &&
        (x*std::sin(yaw)+z*std::cos(yaw))/length>=0.5f;
}
}
