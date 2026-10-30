#include "../dllmain/DirectorCore.h"
#include <cassert>
#include <climits>
#include <iostream>

int main()
{
    aiza::Random first(12345),second(12345),other(54321);
    bool difference=false;
    for(unsigned i=0;i<1000;++i) {
        const auto value=first.Next();assert(value==second.Next());difference|=value!=other.Next();
        assert(first.Pick(7)<7);second.Pick(7);
    }assert(difference);
    const auto saw=aiza::TemplateKey(0x15,4,2,0x10000020);
    assert(saw==aiza::TemplateKey(0x15,4,2,0x10000037));
    assert(saw!=aiza::TemplateKey(0x15,4,2,0x05000020));
    assert(aiza::ChainsawAppearance(0x15,4,0x10000020));
    assert(!aiza::ChainsawAppearance(0x15,0,0x10000020));
    for(unsigned id:{0,0x13,0x14,0x18,0x1e,0x2a,0x3b,0x3d,0x4e}) assert(!aiza::CatalogTarget(id,true));
    int16_t x,y,z;
    assert(aiza::EncodeESLPosition({100,-200,300},x,y,z) && x==10 && y==-20 && z==30);
    assert(aiza::EncodeESLPosition({327670,-327680,0},x,y,z));
    assert(!aiza::EncodeESLPosition({400000,0,0},x,y,z));
    assert(!aiza::EncodeESLPosition({NAN,0,0},x,y,z));
    assert(aiza::WaveCount(3,2,1)==3 && aiza::WaveCount(3,2,2)==5);
    assert(aiza::WaveCount(INT_MAX,INT_MAX,INT_MAX)==20);
    assert(aiza::WaveCount(3,2,INT_MIN)==3);
    assert(aiza::WaveHPFactor(INT_MAX,2)==15 && aiza::WaveHPFactor(INT_MIN,2)==1);
    assert(aiza::FrontCone({0,0,0},{0,0,500},0,1200));
    assert(!aiza::FrontCone({0,0,0},{0,0,-500},0,1200));
    assert(!aiza::FrontCone({0,0,0},{0,1000,500},0,1200));
    assert(!aiza::FrontCone({0,0,0},{0,0,1300},0,1200));
    std::cout << "Director seed, template identity, position encoding, wave bounds and bowling cone: PASS\n";
}
