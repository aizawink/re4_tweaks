#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace aiza
{
inline std::int16_t ScaleEnemyHP(std::int16_t hp, float multiplier)
{
    // Zero/negative HP may represent native death or sentinel states.
    if (hp <= 0 || !std::isfinite(multiplier) || multiplier <= 0.0f)
        return hp;
    const double scaled = static_cast<double>(hp) * multiplier;
    return static_cast<std::int16_t>(std::clamp(scaled, 1.0, 32767.0));
}

inline float ClampVillageScale(float scale)
{
    return std::isfinite(scale) ? std::clamp(scale, 0.5f, 2.0f) : 1.0f;
}

inline bool IsVillageSizeCandidate(unsigned room, unsigned id, int hp)
{
    // Deliberately restricted to the village room and the Ganado ID
    // observed in the user's ESL captures. This does not identify chainsaws.
    return room == 0x0101 && id == 0x15 && hp > 0;
}
}
