#include "../dllmain/ChaosCore.h"
#include <cassert>
#include <iostream>
int main()
{
    for (int mode=0; mode<3; ++mode)
    {
        for (float dt : {1.0f/30, 1.0f/60, 1.0f/144})
        {
            aiza::Flight flight;
            flight.Start({100,200,300}, {0,0,0}, mode, 12000, 4, 3000);
            float maximum=200;
            int frames=0;
            while(flight.active && frames++<1000)
            {
                const auto position=flight.Step(dt);
                assert(aiza::Finite(position));
                assert(position.y>=200 && position.y<=12201);
                const float x=position.x-100, z=position.z-300;
                assert(x*x+z*z<=3001.0f*3001.0f);
                maximum=std::max(maximum, position.y);
            }
            assert(!flight.active && maximum>12180);
            assert(flight.position.x==100 && flight.position.y==200 && flight.position.z==300);
            assert(std::abs(flight.age-4)<=dt*2);
        }
    }
    aiza::Flight flight;
    flight.Start({0,0,0}, {0,0,0}, 2, 30000, 8, 4000);
    flight.Step(.1f); flight.Cancel();
    assert(!flight.active && flight.position.y==0);
    flight.Start({0,0,0}, {0,0,0}, 9, std::nanf(""), -1, INFINITY);
    assert(flight.duration==.5f && flight.height==6000 && flight.spread==1200);
    flight.Step(std::nanf(""));
    assert(flight.age==0 && aiza::Finite(flight.position));
    assert(aiza::IsChaosTarget(0x15, false));
    for(unsigned id: {0u,3u,4u,5u,0x13u,0x14u,0x18u,0x1Eu,0x2Fu,0x35u,0x38u})
        assert(!aiza::IsChaosTarget(id, true));
    assert(!aiza::IsChaosTarget(0x2B, false));
    assert(aiza::IsChaosTarget(0x2B, true));
    assert(aiza::IsChaosGun(35));
    assert(!aiza::IsChaosGun(1) && !aiza::IsChaosGun(14) && !aiza::IsChaosGun(53));
    std::cout << "Flight trajectories, return/cancel, frame rates, target/gun filters: PASS\n";
}
