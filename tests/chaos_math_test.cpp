#include "../dllmain/ChaosMath.h"
#include <cassert>
#include <limits>
#include <iostream>
int main()
{
    assert(aiza::ScaleEnemyHP(5000, 15.0f) == 32767);
    assert(aiza::ScaleEnemyHP(1200, 5.0f) == 6000);
    assert(aiza::ScaleEnemyHP(1, 0.1f) == 1);
    for (int value = -32768; value <= 32767; ++value)
    {
        const auto hp = static_cast<std::int16_t>(value);
        assert(aiza::ScaleEnemyHP(hp, 1.0f) == hp);
        for (float factor : {0.1f, 0.3f, 5.0f, 15.0f, 100000.0f})
        {
            const int actual = aiza::ScaleEnemyHP(hp, factor);
            if (value <= 0) assert(actual == value);
            else
            {
                const double product = double(value) * factor;
                assert(actual == (product < 1 ? 1 : product > 32767 ? 32767 : int(product)));
            }
        }
        assert(aiza::ScaleEnemyHP(hp, std::numeric_limits<float>::quiet_NaN()) == hp);
        assert(aiza::ScaleEnemyHP(hp, std::numeric_limits<float>::infinity()) == hp);
        assert(aiza::ScaleEnemyHP(hp, -1) == hp);
    }
    assert(aiza::ClampVillageScale(0.1f) == 0.5f);
    assert(aiza::ClampVillageScale(4.0f) == 2.0f);
    assert(aiza::ClampVillageScale(1.35f) == 1.35f);
    assert(aiza::ClampVillageScale(std::numeric_limits<float>::quiet_NaN()) == 1.0f);
    assert(aiza::IsVillageSizeCandidate(0x101, 0x15, 1200));
    assert(!aiza::IsVillageSizeCandidate(0x100, 0x15, 1200));
    assert(!aiza::IsVillageSizeCandidate(0x101, 0x18, 1200));
    assert(!aiza::IsVillageSizeCandidate(0x101, 0x2B, 1200));
    assert(!aiza::IsVillageSizeCandidate(0x101, 0x15, 0));
    std::cout << "Chaos math and village eligibility: PASS\n";
}
